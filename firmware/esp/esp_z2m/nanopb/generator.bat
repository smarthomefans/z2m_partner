cd /d %~dp0

D:\Workspaces_Smarthome\esp\nanopb\generator-bin\protoc.exe --nanopb_out=. Ser2NetConfig.proto
copy Ser2NetConfig.pb.h ..\include /y
copy Ser2NetConfig.pb.c ..\src /y
del Ser2NetConfig.pb.h
del Ser2NetConfig.pb.c
