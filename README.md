# RECOVERY-IA — bot Telegram em C++

Esta é a primeira versão do bot. Ela conecta à Telegram Bot API, responde `/start` e `/help` e confirma o recebimento de links HTTP/HTTPS.

**Importante:** nesta versão os links ainda não são escaneados. Os módulos WhatWeb, Wappalyzer, Wayback e HTTrack serão integrados separadamente. Acesso a cPanel, SSH/SFTP, bancos de dados e arquivos executáveis exige autorização e credenciais/arquivos apropriados.

## Arquivos

- `src/main.cpp` — código C++ do bot.
- `Dockerfile` — ambiente de compilação e execução para Render.
- `.gitignore` — evita publicar segredos e binários.

## Deploy manual no Render

1. Envie o conteúdo deste ZIP para a raiz de um repositório GitHub.
2. No Render, selecione **New → Background Worker**.
3. Conecte o repositório e escolha a branch `main`.
4. Escolha **Docker** como runtime/ambiente e mantenha `Dockerfile` como caminho do Dockerfile.
5. Em **Environment**, crie a variável `TELEGRAM_TOKEN` e coloque o token do BotFather no campo Value.
6. Salve e faça o deploy.
7. Abra os logs. Deve aparecer `Conectado como @... Aguardando mensagens...`.

## Teste local opcional no Termux

Dependências:
```bash
pkg update
pkg install clang libcurl nlohmann-json
```

Compile:
```bash
g++ -std=c++17 src/main.cpp -o recovery \
  $(pkg-config --cflags --libs libcurl)
```

Configure o token apenas na sessão local:
```bash
export TELEGRAM_TOKEN='COLOQUE_SEU_TOKEN_AQUI'
./recovery
```

Não envie o token para o GitHub, não o coloque no código e não o compartilhe em conversas. Se ele for exposto, revogue-o pelo BotFather e gere outro.

## Observações

- O bot usa long polling (`getUpdates`), portanto não configure webhook para esta versão.
- Mantenha apenas uma instância do bot executando.
- Se houver webhook antigo, remova-o antes de iniciar o worker:
```bash
curl -sS "https://api.telegram.org/bot${TELEGRAM_TOKEN}/deleteWebhook"
```
- O worker precisa permanecer ativo. Verifique os limites e custos do plano Render escolhido.
