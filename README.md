# Router (P4.Redes)

Resumo
- Programa em C que determina a interface de saída para um pacote dado, usando Longest Prefix Match sobre uma tabela de reenvío (routing table) lida a partir de ficheiro.
- Recebe dois argumentos na linha de comandos: o ficheiro de rotas e a IP de origem (nessa ordem).
- Se não houver coincidência, usa a interface por defeito `0`.

Objetivo
- Praticar leitura/parse de tabelas de reencaminhamento e implementação de correspondência por maior prefixo (longest-prefix match).

Formato do ficheiro de rotas
- Cada linha: `rede/prefixo,interfaz`
- A rede pode usar notação curta com 1..4 octetos, ex.: `194.64/16,2` ou `194.64.0.0/16,2`
- Exemplo (arquivo `rutas.txt` que acompanha o projeto):
  ```
  194.32.0.0/11,1
  194.64/16,2
  194.64.0.0/10,3
  194.128.0.0/9,3
  195.0.0.0/9,3
  0.0.0.0/0,0
  ```

Compilar & executar (Windows PowerShell)
- Com gcc (MinGW/MSYS2):
  ```powershell
  gcc -o router.exe router.c -lresolv
  .\router.exe "c:\Users\mathe\Downloads\P4.Redes\rutas.txt" 194.64.5.6
  ```
- Se `-lresolv` produzir erro, tente sem ele:
  ```powershell
  gcc -o router.exe router.c
  ```

Exemplos de saída esperada
- `.\router.exe rutas.txt 194.64.5.6`
  - Esperado: `Interfaz de salida: 2, Prefijo: 16` (corresponde a 194.64.0.0/16)
- `.\router.exe rutas.txt 194.33.10.5`
  - Esperado: `Interfaz de salida: 1, Prefijo: 11`
- `.\router.exe rutas.txt 200.1.1.1`
  - Esperado: `Interfaz de salida: 0, Prefijo: 0` (rota por defeito)

Partes importantes do código
- convertir_a_uint32(const char *): aceita 1..4 octetos (p.ex. `194.64`) e constrói a IP em inteiro de 32 bits (ordem host).
- crear_mascarilla(int): gera a máscara por prefixo para comparação de redes.
- cargar_rutas(const char *, Ruta *): lê o ficheiro linha a linha, faz trim de CR/LF, parse `rede/prefixo,interfaz`, preenche a tabela.
  - Atualmente imprime linhas de depuração `Cargada: ...` e usa `sleep(2)` entre entradas — remova ou comente estas duas linhas para execução normal.
- main: converte a IP de entrada, faz a busca por melhor coincidência (maior prefixo) e imprime interface e prefixo.

Depuração e problemas comuns
- Saída `Interfaz de salida: 0, Prefijo: 0` significa que não foi encontrada rota mais específica — verifique:
  - Caminho do ficheiro está correto e legível.
  - Formato das linhas está correto (vírgula e `/prefixo`).
  - CRLF em ficheiros Windows normalmente tratado, mas se linhas não aparecem, verifique conteúdo com um editor.
- Se ao compilar houver erros sobre `-lresolv`, tente compilar sem essa opção.
- Para ver as rotas carregadas use as mensagens de depuração (`Cargada: ...`). Remova `sleep(2)` para não atrasar a execução.

Sugestões rápidas
- Para testes, modifique temporariamente `cargar_rutas` para não dormir e para imprimir o número total de rotas carregadas.
- Para comportamento final, remova prints de depuração e a chamada `sleep`.

Licença / Observações
- Ficheiro de exemplo e código são para fins de aprendizagem e exercícios de redes.