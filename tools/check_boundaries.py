#!/usr/bin/env python3
"""Guard the hardware/application boundary of the published source tree.

This checks architecture and accidental literal credentials; it is not a
substitute for source review or physical hardware validation.
"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
ARDUINO=(ROOT/'platformio.ini').exists()
BSP=ROOT/('lib/bsp/src' if ARDUINO else 'components/bsp')
BASE=ROOT/('src' if ARDUINO else 'main')
source_ext={'.c','.cpp','.h','.hpp','.cc'}
errors=[]
# Network protocol implementations and product application owners never belong
# in BSP or default main. Low-level opt-in PPP is intentionally board transport.
for directory in [BSP,BASE]:
 for p in directory.rglob('*'):
  if p.suffix not in source_ext:continue
  text=p.read_text()
  for pattern in [r'Application::GetInstance',r'esp_mqtt_client_',r'PubSubClient',r'esp_ota_',r'\bHTTPClient\b',r'\bWiFiManager\b']:
   if re.search(pattern,text):errors.append(f'{p.relative_to(ROOT)}: application dependency {pattern}')
  if re.search(r'(?i)\b(password|passwd|api_key|broker|ssid|access_token)\b\s*[=:]\s*"[^"\n]+"',text):
   errors.append(f'{p.relative_to(ROOT)}: possible embedded credential/endpoint')
main=BASE/'main.cpp'
text=main.read_text()
# Default entry remains a bring-up, not a moved/renamed product application.
if len(text.splitlines())>24:errors.append('default main exceeds the minimal bring-up boundary')
if re.search(r'\b(xTaskCreate|esp_wifi_init|esp_blufi_profile_init|bsp_4g_start|bsp_nfc_start|audio_begin|bsp_audio_open)\s*\(',text):
 errors.append('default main starts diagnostic/application services')
for path in ['LICENSE','AGENTS.md','AGENTS.zh_CN.md','docs/BSP.md','docs/BSP.zh_CN.md','examples/hardware-demo']:
 if not (ROOT/path).exists():errors.append(f'missing {path}')
for p in (ROOT/'components').glob('**/idf_component.yml'):
 for url in re.findall(r'https://github.com/FoloToy/[^\s]+',p.read_text()):
  if url.rstrip('.git')!='https://github.com/FoloToy/folotoy-firmware-driver-open-source':
   errors.append(f'{p.relative_to(ROOT)}: unexpected FoloToy dependency {url}')
if errors:raise SystemExit('\n'.join(errors))
print('PASS: minimal default entry, BSP/application boundary, literal-credential guard, public dependency URLs')
