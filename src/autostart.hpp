#pragma once

namespace magic_remap::autostart {

// Estado: registro HKCU\Software\Microsoft\Windows\CurrentVersion\Run\MagicRemap
// aponta para o exe atual?
bool IsEnabled();

// Habilita ou desabilita autostart no logon do usuário atual.
// Retorna true em sucesso.
bool Set(bool enable);

}  // namespace magic_remap::autostart
