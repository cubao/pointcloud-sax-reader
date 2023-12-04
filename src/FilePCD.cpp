// based on open3d/cpp/open3d/io/file_format/FilePCD.cpp

// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// Copyright (c) 2018-2023 www.open3d.org
// SPDX-License-Identifier: MIT
// ----------------------------------------------------------------------------

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

extern "C" {
#include "liblzf/lzf.h"
}
#include "pointcloud_sax_reader.hpp"

#define DEFAULT_IO_BUFFER_SIZE 1024
#include <functional>

namespace open3d {
enum PCDDataType { PCD_DATA_ASCII = 0, PCD_DATA_BINARY = 1, PCD_DATA_BINARY_COMPRESSED = 2 };

struct PCLPointField {
public:
  std::string name;
  int size;
  char type;
  int count;
  // helper variable
  int count_offset;
  int offset;
};

struct PCDHeader {
public:
  std::string version;
  std::vector<PCLPointField> fields;
  int width;
  int height;
  int points;
  PCDDataType datatype;
  int elementnum;
  int pointsize;
};

bool CheckHeader(PCDHeader& header) {
  if (header.points <= 0 || header.pointsize <= 0) {
    std::cerr << "[CheckHeader] PCD has no data." << std::endl;
    return false;
  }
  if (header.fields.size() == 0 || header.pointsize <= 0) {
    std::cerr << "[CheckHeader] PCD has no fields." << std::endl;
    return false;
  }
  bool has_x = false;
  bool has_y = false;
  bool has_z = false;
  for (const auto& field : header.fields) {
    if (field.name == "x") {
      has_x = true;
    } else if (field.name == "y") {
      has_y = true;
    } else if (field.name == "z") {
      has_z = true;
    }
  }
  bool has_points = (has_x && has_y && has_z);
  if (!has_points) {
    std::cerr << "[CheckHeader] Fields for point data are not complete." << std::endl;
    return false;
  }
  return true;
}

inline std::vector<std::string> SplitString(const std::string& str,
                                            const std::string& delimiters = " ",
                                            bool trim_empty_str = true) {
  std::vector<std::string> tokens;
  std::string::size_type pos = 0, new_pos = 0, last_pos = 0;
  while (pos != std::string::npos) {
    pos = str.find_first_of(delimiters, last_pos);
    new_pos = (pos == std::string::npos ? str.length() : pos);
    if (new_pos != last_pos || !trim_empty_str) {
      tokens.push_back(str.substr(last_pos, new_pos - last_pos));
    }
    last_pos = new_pos + 1;
  }
  return tokens;
}

inline bool ReadPCDHeader(FILE* file, PCDHeader& header) {
  char line_buffer[DEFAULT_IO_BUFFER_SIZE];
  size_t specified_channel_count = 0;

  while (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, file)) {
    std::string line(line_buffer);
    if (line == "") {
      continue;
    }
    std::vector<std::string> st = SplitString(line, "\t\r\n ");
    std::stringstream sstream(line);
    sstream.imbue(std::locale::classic());
    std::string line_type;
    sstream >> line_type;
    if (line_type.substr(0, 1) == "#") {
    } else if (line_type.substr(0, 7) == "VERSION") {
      if (st.size() >= 2) {
        header.version = st[1];
      }
    } else if (line_type.substr(0, 6) == "FIELDS" || line_type.substr(0, 7) == "COLUMNS") {
      specified_channel_count = st.size() - 1;
      if (specified_channel_count == 0) {
        std::cerr << "[ReadPCDHeader] Bad PCD file format." << std::endl;
        return false;
      }
      header.fields.resize(specified_channel_count);
      int count_offset = 0, offset = 0;
      for (size_t i = 0; i < specified_channel_count; i++, count_offset += 1, offset += 4) {
        header.fields[i].name = st[i + 1];
        header.fields[i].size = 4;
        header.fields[i].type = 'F';
        header.fields[i].count = 1;
        header.fields[i].count_offset = count_offset;
        header.fields[i].offset = offset;
      }
      header.elementnum = count_offset;
      header.pointsize = offset;
    } else if (line_type.substr(0, 4) == "SIZE") {
      if (specified_channel_count != st.size() - 1) {
        std::cerr << "[ReadPCDHeader] Bad PCD file format." << std::endl;
        return false;
      }
      int offset = 0, col_type = 0;
      for (size_t i = 0; i < specified_channel_count; i++, offset += col_type) {
        sstream >> col_type;
        header.fields[i].size = col_type;
        header.fields[i].offset = offset;
      }
      header.pointsize = offset;
    } else if (line_type.substr(0, 4) == "TYPE") {
      if (specified_channel_count != st.size() - 1) {
        std::cerr << "[ReadPCDHeader] Bad PCD file format." << std::endl;
        return false;
      }
      for (size_t i = 0; i < specified_channel_count; i++) {
        header.fields[i].type = st[i + 1].c_str()[0];
      }
    } else if (line_type.substr(0, 5) == "COUNT") {
      if (specified_channel_count != st.size() - 1) {
        std::cerr << "[ReadPCDHeader] Bad PCD file format." << std::endl;
        return false;
      }
      int count_offset = 0, offset = 0, col_count = 0;
      for (size_t i = 0; i < specified_channel_count; i++) {
        sstream >> col_count;
        header.fields[i].count = col_count;
        header.fields[i].count_offset = count_offset;
        header.fields[i].offset = offset;
        count_offset += col_count;
        offset += col_count * header.fields[i].size;
      }
      header.elementnum = count_offset;
      header.pointsize = offset;
    } else if (line_type.substr(0, 6) == "POINTS") {
      sstream >> header.points;
    } else if (line_type.substr(0, 4) == "DATA") {
      header.datatype = PCD_DATA_ASCII;
      if (st.size() >= 2) {
        if (st[1].substr(0, 17) == "binary_compressed") {
          header.datatype = PCD_DATA_BINARY_COMPRESSED;
        } else if (st[1].substr(0, 6) == "binary") {
          header.datatype = PCD_DATA_BINARY;
        }
      }
      break;
    }
  }
  if (!CheckHeader(header)) {
    return false;
  }
  header.fields.erase(std::remove_if(header.fields.begin(), header.fields.end(),
                                     [](const PCLPointField& f) -> bool {
                                       return f.name != "x" && f.name != "y" && f.name != "z";
                                     }),
                      header.fields.end());
  std::sort(header.fields.begin(), header.fields.end(),
            [](const PCLPointField& f1, const PCLPointField& f2) { return f1.name < f2.name; });
  return true;
}

inline double UnpackBinaryPCDElement(const char* data_ptr, const char type, const int size) {
  if (type == 'I') {
    if (size == 1) {
      std::int8_t data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else if (size == 2) {
      std::int16_t data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else if (size == 4) {
      std::int32_t data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else {
      return 0.0;
    }
  } else if (type == 'U') {
    if (size == 1) {
      std::uint8_t data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else if (size == 2) {
      std::uint16_t data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else if (size == 4) {
      std::uint32_t data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else {
      return 0.0;
    }
  } else if (type == 'F') {
    if (size == 4) {
      float data;
      memcpy(&data, data_ptr, sizeof(data));
      return (double)data;
    } else {
      return 0.0;
    }
  }
  return 0.0;
}

inline double UnpackASCIIPCDElement(const char* data_ptr, const char type, const int size) {
  char* end;
  if (type == 'I') {
    return (double)std::strtol(data_ptr, &end, 0);
  } else if (type == 'U') {
    return (double)std::strtoul(data_ptr, &end, 0);
  } else if (type == 'F') {
    return std::strtod(data_ptr, &end);
  }
  return 0.0;
}

inline bool ReadPCDData(FILE* file, const PCDHeader& header,
                        const std::function<void(double, double, double)>& point_callback) {
  // The header should have been checked
  auto& field_x = header.fields[0];
  auto& field_y = header.fields[1];
  auto& field_z = header.fields[2];
  if (header.datatype == PCD_DATA_ASCII) {
    char line_buffer[DEFAULT_IO_BUFFER_SIZE];
    int idx = 0;
    while (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, file) && idx < header.points) {
      std::string line(line_buffer);
      std::vector<std::string> strs = SplitString(line, "\t\r\n ");
      if ((int)strs.size() < header.elementnum) {
        continue;
      }
      double x =
          UnpackASCIIPCDElement(strs[field_x.count_offset].c_str(), field_x.type, field_x.size);
      double y =
          UnpackASCIIPCDElement(strs[field_y.count_offset].c_str(), field_y.type, field_y.size);
      double z =
          UnpackASCIIPCDElement(strs[field_z.count_offset].c_str(), field_z.type, field_z.size);
      point_callback(x, y, z);
      idx++;
    }
    return true;
  } else if (header.datatype == PCD_DATA_BINARY) {
    std::unique_ptr<char[]> buffer(new char[header.pointsize]);
    for (int i = 0; i < header.points; i++) {
      if (fread(buffer.get(), header.pointsize, 1, file) != 1) {
        std::cerr << "[ReadPCDData] Failed to read data record." << std::endl;
        return false;
      }
      double x = UnpackBinaryPCDElement(buffer.get() + field_x.offset, field_x.type, field_x.size);
      double y = UnpackBinaryPCDElement(buffer.get() + field_y.offset, field_y.type, field_y.size);
      double z = UnpackBinaryPCDElement(buffer.get() + field_z.offset, field_z.type, field_z.size);
      point_callback(x, y, z);
    }
    return true;
  } else if (header.datatype == PCD_DATA_BINARY_COMPRESSED) {
    std::uint32_t compressed_size;
    std::uint32_t uncompressed_size;
    if (fread(&compressed_size, sizeof(compressed_size), 1, file) != 1) {
      std::cerr << "[ReadPCDData] Failed to read data record." << std::endl;
      return false;
    }
    if (fread(&uncompressed_size, sizeof(uncompressed_size), 1, file) != 1) {
      std::cerr << "[ReadPCDData] Failed to read data record." << std::endl;
      return false;
    }
    std::unique_ptr<char[]> buffer_compressed(new char[compressed_size]);
    if (fread(buffer_compressed.get(), 1, compressed_size, file) != compressed_size) {
      std::cerr << "[ReadPCDData] Failed to read data record." << std::endl;
      return false;
    }
    std::unique_ptr<char[]> buffer(new char[uncompressed_size]);
    if (lzf_decompress(buffer_compressed.get(), (unsigned int)compressed_size, buffer.get(),
                       (unsigned int)uncompressed_size) != uncompressed_size) {
      std::cerr << "[ReadPCDData] Uncompression failed." << std::endl;
      return false;
    }
    for (int i = 0; i < header.points; i++) {
      double x = UnpackBinaryPCDElement(
          buffer.get() + field_x.offset * header.points + i * field_x.size * field_x.count,
          field_x.type, field_x.size);
      double y = UnpackBinaryPCDElement(
          buffer.get() + field_y.offset * header.points + i * field_y.size * field_y.count,
          field_y.type, field_y.size);
      double z = UnpackBinaryPCDElement(
          buffer.get() + field_z.offset * header.points + i * field_z.size * field_z.count,
          field_z.type, field_z.size);
      point_callback(x, y, z);
    }
    return true;
  }
  return false;
}

}  // namespace open3d

namespace cubao {
using namespace open3d;
bool pointcloud_sax_read(const std::string& filename,
                         const std::function<void(double, double, double)>& point_callback) {
  PCDHeader header;
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == NULL) {
    std::cerr << "Read PCD failed: unable to open file: " << filename << std::endl;
    return false;
  }
  if (!ReadPCDHeader(file, header)) {
    std::cerr << "Read PCD failed: unable to parse header." << std::endl;
    fclose(file);
    return false;
  }
  if (!ReadPCDData(file, header, point_callback)) {
    std::cerr << "Read PCD failed: unable to read data." << std::endl;
    fclose(file);
    return false;
  }
  fclose(file);
  return true;
}

}  // namespace cubao
