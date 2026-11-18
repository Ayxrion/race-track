import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';

// Compile the actual geometry, cars, race director and championship code. Only
// device/render/network boundaries are replaced; no second physics model.
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = name => fs.readFileSync(path.join(root, 'race_track', name), 'utf8').replace(/\r\n/g, '\n');
function between(text, from, to) {
  const start = text.indexOf(from), end = text.indexOf(to, start);
  if (start < 0 || end < 0) throw new Error(`Missing source boundary: ${from} / ${to}`);
  return text.slice(start, end);
}
const strip = text => text.replace(/^#pragma once\s*$/gm, '').replace(/^#include "(?:common|effects|cars|settings)\.h"\s*$/gm, '');
const build = path.join(root, 'build', 'simulation');
fs.mkdirSync(build, { recursive: true });
const source = [
  '#include <cstdint>\n' + between(read('weather.h'), 'enum WeatherCondition', 'static const char* const WEATHER_NAMES'),
  fs.readFileSync(path.join(root, 'tests', 'sim-platform.h'), 'utf8'),
  between(read('common.h'), 'static inline float clampf', '// ─── Colours'),
  strip(read('track.h')),
  between(read('world.h'), '#ifndef GRID_GAP', '// ───'),
  between(read('world.h'), 'static int8_t   weatherOverride', 'static void updateLighting'),
  'static void testSurfaceWeather() { const bool snap=false; const auto wx=effectiveWeather(); const float wetT=wetTarget(wx);\n' +
    between(read('world.h'), '  snowCover = lerpf', '\n\n  uint32_t daySky') + '\n}',
  strip(read('cars.h').split('#include "replay.h"')[0]),
  strip(read('race.h')),
  strip(read('champ.h')),
  fs.readFileSync(path.join(root, 'tests', 'simulation.cpp'), 'utf8'),
].join('\n');
fs.writeFileSync(path.join(build, 'simulation.cpp'), source);
const exe = path.join(build, process.platform === 'win32' ? 'simulation.exe' : 'simulation');
if (process.platform === 'win32') {
  const vc = process.env.VCVARS64 || 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat';
  fs.writeFileSync(path.join(build, 'compile.cmd'), `@echo off\r\ncall "${vc}" >nul\r\nif errorlevel 1 exit /b 1\r\ncl /nologo /EHsc /O2 /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS /I"${path.join(root, 'race_track')}" simulation.cpp /Fe:simulation.exe\r\n`);
  execFileSync('cmd.exe', ['/d', '/c', 'compile.cmd'], { cwd: build, stdio: 'inherit' });
} else {
  execFileSync(process.env.CXX || 'g++', ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Wno-unused-function', '-Wno-unused-variable', '-I', path.join(root, 'race_track'), path.join(build, 'simulation.cpp'), '-o', exe], { stdio: 'inherit' });
}
execFileSync(exe, process.argv.slice(2), { cwd: root, stdio: 'inherit' });
