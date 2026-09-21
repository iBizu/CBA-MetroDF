// Hook do core ESP32 (esp32-hal-misc.c, chamado antes do setup()).
// Retornando true, o core NÃO marca a imagem recém-gravada por OTA como válida no boot;
// quem faz isso é verificarEstadoOTA() em ota_update.ino, depois da inicialização básica.
// Se o firmware novo travar antes de chegar lá, o bootloader volta para a imagem anterior.
//
// Fica num .cpp (e não num .ino) porque o pré-processador do Arduino gera protótipos C++
// para toda função definida em .ino, o que conflita com a ligação "C" exigida pelo hook.
extern "C" bool verifyRollbackLater()
{
  return true;
}
