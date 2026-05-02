# MagicRemap

Win32 app que faz o **Apple Magic Keyboard** se comportar de forma natural no Windows. Detecta o teclado pelo VID HID `0x05AC` e remapeia **só as teclas vindas dele** — qualquer outro teclado plugado continua funcionando normalmente.

## O que ele faz

- `⌘ Cmd` (esquerdo e direito) passa a agir como `Ctrl` → `Cmd+C`, `Cmd+V`, `Cmd+Z`, `Cmd+A`, `Cmd+S`, `Cmd+T`, `Cmd+W`, etc. funcionam direto.
- Atalhos com tradução semântica:
  - `Cmd+Tab` → `Alt+Tab` (troca de app)
  - `Cmd+Q` → `Alt+F4` (fechar)
  - `Cmd+Space` → tecla `Win` (abre busca/Iniciar)
  - `Cmd+←/→` → `Home`/`End`
  - `Cmd+↑/↓` → `Ctrl+Home`/`Ctrl+End`
  - `Option+←/→` → `Ctrl+←/→` (pular palavra)
  - `Option+Backspace` → `Ctrl+Backspace`
  - `Cmd+Backspace` → apaga até o início da linha
- Ícone na bandeja com toggle, autostart e reload de config.
- Single-instance (mutex nomeado).
- **Tecla de pânico**: `Shift+Ctrl+Alt+F12` encerra o app imediatamente caso algo trave.

## Build

Pré-requisitos: **Visual Studio 2022** (workload "Desktop development with C++") em uma máquina Windows.

```powershell
# Pelo IDE: abra magic_remap.sln, selecione Release|x64, build (F7).

# Ou pela linha de comando:
msbuild magic_remap.sln /p:Configuration=Release /p:Platform=x64
# Saída: x64\Release\magic_remap.exe
```

### Ícone (opcional)

O projeto referencia `res/magic_remap.ico` mas o arquivo não está incluído. Para adicionar um ícone próprio:

1. Coloque um `.ico` em `res/magic_remap.ico`.
2. Descomente a linha `IDI_APPICON ICON ...` em `res/magic_remap.rc`.
3. Rebuild.

Sem ícone customizado, o app usa o ícone padrão de aplicação do Windows.

## Instalador

Pré-requisitos: [Inno Setup 6](https://jrsoftware.org/isinfo.php).

```powershell
# Após o build do exe:
iscc installer\magic_remap.iss
# Saída: installer\Output\MagicRemapSetup-1.0.0.exe
```

O instalador:
- Coloca o app em `Program Files\MagicRemap` (ou no perfil do usuário se sem admin).
- Cria atalho no Menu Iniciar.
- Oferece checkbox para iniciar automaticamente com o Windows.
- O instalador **não** é assinado — o SmartScreen vai mostrar um aviso na primeira execução. Clique em "Mais informações" → "Executar mesmo assim".

## Configuração

`%APPDATA%\MagicRemap\config.ini` é criado na primeira execução com os defaults:

```ini
[General]
enabled=true
autostart=false
swap_ctrl_too=false   ; se true, também faz Ctrl→Win (mais "mac-like" mas perde Win key)
verbose=false

[Shortcuts]
cmd_tab=true
cmd_q=true
cmd_space=true
cmd_arrows=true
option_arrows=true
cmd_backspace=true
option_backspace=true

[Devices]
; lista CSV de VIDs HID a tratar como Apple. Padrão: 05AC (Apple).
; adicione se seu teclado reportar outro VID (verifique no Gerenciador de Dispositivos).
vendor_ids=05AC,004C
```

Edite, salve, e clique direito no tray → **Recarregar config**.

## Como funciona

Diferente de SharpKeys, AutoHotKey ou PowerToys Keyboard Manager, o MagicRemap usa a **Raw Input API** com `RIDEV_NOLEGACY`, o que permite:

1. Identificar o dispositivo de origem de cada tecla (`hDevice` → caminho HID → VID).
2. Suprimir a entrega original ao app em foco.
3. Reinjetar a tecla via `SendInput`, original ou remapeada conforme a origem.

Eventos sintetizados por `SendInput` não disparam `WM_INPUT`, então não há loop.

## Limitações

- A `fn` key da Apple e os atalhos de brilho/mídia (`F1`–`F12` da Apple) não são tratados na V1 — requer parsing de HID report descriptor.
- App em foco rodando como Administrador ignora `SendInput` vindo de processo não-elevado (UIPI). Workaround: rodar o MagicRemap como Admin, ou desabilitar via tray.
- Não há UI gráfica de configuração; só o INI.

## Tecla de pânico

Se algo travar e o teclado parar de responder, segure **Shift + Ctrl + Alt + F12** no Apple Magic Keyboard (ou em qualquer outro teclado conectado). O app encerra e o Windows volta ao comportamento normal.

Alternativas: encerrar `magic_remap.exe` pelo Gerenciador de Tarefas (`Ctrl+Shift+Esc`), ou desplugar/desparear o teclado.

## Licença

MIT — veja [LICENSE](LICENSE).
