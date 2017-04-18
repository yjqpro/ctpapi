#ifndef FOLLOW_TRADE_SERVER_UTIL_H
#define FOLLOW_TRADE_SERVER_UTIL_H

std::string GetExecuableFileDirectoryPath();

std::string MakeDataBinaryFileName(const std::string& slave_account, const std::string& sub_dir);

void ClearUpCTPFolwDirectory();

#endif // FOLLOW_TRADE_SERVER_UTIL_H
