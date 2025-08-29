# CubeOpenGL - Instruções para dependências (glue, glm) via vcpkg

## Instalação do vcpkg (se ainda não tiver)
1. Baixe e extraia o vcpkg: https://github.com/microsoft/vcpkg
2. No terminal, execute:
   ```
   bootstrap-vcpkg.bat
   ```

## Instalando as dependências

Abra o terminal do Windows (cmd.exe) e execute:

```
vcpkg install glue glm
```

- Isso instalará as bibliotecas glue e glm para x64-windows.

## Integrando vcpkg ao Visual Studio

1. Execute:
   ```
   vcpkg integrate install
   ```
   Isso permite que o Visual Studio encontre automaticamente as bibliotecas instaladas via vcpkg.

2. Certifique-se de que o triplet está correto (x64-windows). O caminho dos includes será algo como:
   ```
   C:\vcpkg\installed\x64-windows\include
   ```

## Corrigindo erros de include

- Se ainda aparecer erro de "glue.h" não encontrado:
  - Confirme se o diretório de include do vcpkg está em "Additional Include Directories" nas propriedades do projeto.
  - Confirme se a instalação do glue foi bem-sucedida e o arquivo glue.h está em `C:\vcpkg\installed\x64-windows\include`.

## Observação

- O pacote glue pode não estar disponível no repositório oficial do vcpkg. Se não encontrar, baixe manualmente de https://github.com/sgorsten/libraries/tree/master/glue e coloque glue.h e glue.c na pasta do projeto, adicionando glue.c ao projeto no Visual Studio.
