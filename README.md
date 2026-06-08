# BDGB — Binary Doodle Glifo Base

Motor de busqueda semantica basado en glifos. Sistema de nodos 8-bit con dinamica Kaprekar, NLP integrado, cifrado BDGB-Kaprekar, y arquitectura glifo (Primo / Semilla / Comun).

## Compilar

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Test

```bash
build\bdgb_test.exe
```

## Comandos principales

```bash
bdgb                          # modo interactivo
bdgb --search "belleza simetrico"
bdgb --glifo-list
bdgb --glifo-run primo
bdgb --encrypt in.txt out.bdgb clave
bdgb --decrypt out.bdgb dec.txt clave
bdgb --hash "texto"
bdgb --learn "texto para aprender"
bdgb --show-terms
```

## Arquitectura

Ver `glifos/README.md` para la documentacion completa del sistema de glifos, clasificacion, reglas y composicion de sistemas.
