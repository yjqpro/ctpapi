#include "util.h"
#include <windows.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "follow_strategy_mode/string_util.h"

boost::filesystem::path CreateDirectoryIfNoExist(const std::string& sub_dir) {
  boost::filesystem::path dir{GetExecuableFileDirectoryPath()};
  dir /= sub_dir;
  if (!boost::filesystem::exists(dir) &&
      !boost::filesystem::create_directory(dir)) {
    std::cout << "create directory error\n";
  }
  return dir;
}

std::string GetExecuableFileDirectoryPath() {
  char path_buffer[MAX_PATH] = {0};
  GetModuleFileNameA(NULL, path_buffer, MAX_PATH);
  boost::filesystem::path file(path_buffer);
  return file.parent_path().string();
}


std::string MakeDataBinaryFileName(const std::string& slave_account,
                                   const std::string& sub_dir) {
  auto time = boost::posix_time::second_clock::local_time();
  struct ::tm tm_time = boost::posix_time::to_tm(time);
  std::ostringstream time_pid_stream;
  time_pid_stream.fill('0');
  time_pid_stream << 1900 + tm_time.tm_year << std::setw(2)
                  << 1 + tm_time.tm_mon << std::setw(2) << tm_time.tm_mday
                  << '-' << std::setw(2) << tm_time.tm_hour << std::setw(2)
                  << tm_time.tm_min << std::setw(2) << tm_time.tm_sec;
  const std::string& time_pid_string = time_pid_stream.str();

  boost::filesystem::path dir = CreateDirectoryIfNoExist(sub_dir);
  return dir.string() + "\\" + slave_account + '-' + time_pid_string + ".bin";
}

void ClearUpCTPFolwDirectory() {
  boost::filesystem::path dir = CreateDirectoryIfNoExist(kCTPFolwPath);
  boost::filesystem::directory_iterator end_iter;
  for (boost::filesystem::directory_iterator it(dir); it != end_iter; ++it) {
    boost::filesystem::remove_all(it->path());
  }
}
