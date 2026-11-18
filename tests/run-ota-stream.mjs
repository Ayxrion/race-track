import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const build=path.join(root,'build','ota-test'); fs.mkdirSync(build,{recursive:true});
const json=process.env.ARDUINOJSON_INCLUDE || (process.platform==='win32' ? path.join(process.env.USERPROFILE,'Documents','Arduino','libraries','ArduinoJson','src') : path.join(root,'build','ArduinoJson','src'));
if(!fs.existsSync(path.join(json,'ArduinoJson.h')))throw Error('Provide ArduinoJson 6.21.6 src through ARDUINOJSON_INCLUDE');
const source=path.join(root,'tests','ota-stream.cpp');
const exe=path.join(build,process.platform==='win32'?'ota-test.exe':'ota-test');
if(process.platform==='win32') {
  const vc=process.env.VCVARS64 || 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat';
  fs.writeFileSync(path.join(build,'compile.cmd'),`@echo off\r\ncall "${vc}" >nul\r\nif errorlevel 1 exit /b 1\r\ncl /nologo /EHsc /O2 /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS /I"${json}" "${source}" /Fe:ota-test.exe\r\n`);
  execFileSync('cmd.exe',['/d','/c','compile.cmd'],{cwd:build,stdio:'inherit'});
} else execFileSync(process.env.CXX||'g++',['-std=c++17','-O2','-Wall','-Wextra','-I',json,source,'-o',exe],{stdio:'inherit'});
execFileSync(exe,[],{stdio:'inherit'});
