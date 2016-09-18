#include "StdAfx.h"
#include "file.h"

std::string File::exec(const std::wstring &directory, std::wstring commandLine, const std::wstring &parameters)
{
  std::string result;

  if (!parameters.empty())
  {
    commandLine += ' ';
    commandLine += parameters;
  }

  PROCESS_INFORMATION procInfo = {0};
  STARTUPINFO startInfo = {0};
  BOOL isSuccess = FALSE;

  HANDLE IN_Rd(0), IN_Wr(0), OUT_Rd(0), OUT_Wr(0), ERR_Rd(0), ERR_Wr(0);

  SECURITY_ATTRIBUTES security;

  security.nLength = sizeof(SECURITY_ATTRIBUTES);
  security.bInheritHandle = TRUE;
  security.lpSecurityDescriptor = NULL;

  CreatePipe(&OUT_Rd, &OUT_Wr, &security, 0);
  CreatePipe(&ERR_Rd, &ERR_Wr, &security, 0);
  CreatePipe(&IN_Rd, &IN_Wr, &security, 0);

  SetHandleInformation(OUT_Rd, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(ERR_Rd, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(IN_Wr, HANDLE_FLAG_INHERIT, 0);

  startInfo.cb = sizeof(STARTUPINFO);
  startInfo.hStdError = ERR_Wr;
  startInfo.hStdOutput = OUT_Wr;
  startInfo.hStdInput = IN_Rd;
  startInfo.dwFlags |= STARTF_USESTDHANDLES;

  isSuccess = CreateProcess(NULL,
    const_cast<wchar_t *>(commandLine.c_str()),  // command line
    NULL,                                        // process security attributes
    NULL,                                        // primary thread security attributes
    TRUE,                                        // handles are inherited
    CREATE_NO_WINDOW,                            // creation flags
    NULL,                                        // use parent's environment
    directory.c_str(),                           // use parent's current directory
    &startInfo,                                  // STARTUPINFO pointer
    &procInfo);                                  // receives PROCESS_INFORMATION

  if (!isSuccess)
  {
    std::string command;
    command.assign(commandLine.begin(), commandLine.end());

    throw Linter::Exception("Linter: Can't execute command: " + command);
  }

  CloseHandle(procInfo.hProcess);
  CloseHandle(procInfo.hThread);

  CloseHandle(ERR_Wr);
  CloseHandle(OUT_Wr);
  CloseHandle(IN_Wr);

  DWORD readBytes;
  std::string buffer;
  buffer.resize(4096);

  for (;;)
  {
    isSuccess = ReadFile(OUT_Rd, &buffer[0], buffer.size(), &readBytes, NULL);
    if (!isSuccess || readBytes == 0)
    {
      break;
    }

    result += std::string(&buffer[0], readBytes);
  }

  return result;
}

std::wstring File::write(const std::wstring &directory, const std::string &data)
{
  if (data.empty())
  {
    return L"";
  }
  TCHAR filename[MAX_PATH] = {0};

  DWORD retVal = GetTempFileName(directory.c_str(), L"deadem", 0, filename);
  if (retVal == 0)
  {
    return L"";
  }

  HANDLE fileHandle = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fileHandle == INVALID_HANDLE_VALUE)
  {
    return L"";
  }

  DWORD bytes(0);
  WriteFile(fileHandle, &data[0], static_cast<DWORD>(data.size() * sizeof(data[0])), &bytes, 0);

  CloseHandle(fileHandle);

  return filename;
}
