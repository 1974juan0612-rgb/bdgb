# Sistema de Glifos

## Arquitectura

Cada **Glifo** pertenece a un **Sistema**. Un Sistema es un **flujo de trabajo** con un proposito definido. Los glifos son los nodos operativos.

Todo sistema tiene un **glifo maestro** (`glifosenilla.json`) en su raiz que define:

- Los glifos que lo componen
- El orden de ejecucion (pipeline)
- Las relaciones y dependencias entre glifos
- Datos extra propios del sistema

```
glifos/
  README.md
  registry.json
  vigilancia-tendencias/         ← sistema
    glifosenilla.json            ← MAESTRO: pipeline, relaciones, config
    README.md
    glifos/
      primo/glifo.json           ← config del glifo nativo
      trend-tracker/             ← codigo del glifo externo
        glifo.json
        trend_tracker.py
        daily/ + weekly/
  youtube-automator/             ← sistema sin glifo maestro aun
    config.json
```

## Orden de creacion

1. **Definir el Sistema** — crear carpeta con `glifosenilla.json`
2. **Crear los Glifos** — cada uno en `glifos/<id>/` con su `glifo.json`

## Tipos de Glifo

| Tipo | Descripcion |
|------|-------------|
| **Nativo** | Compilado en el binario BDGB (`src/glifo.c`). Cero dependencias |
| **Externo** | Script (Python, etc.) dentro de la carpeta del sistema |

## Comandos

```bash
bdgb --glifo-list
bdgb --glifo-run primo
python3 glifos/vigilancia-tendencias/glifos/trend-tracker/trend_tracker.py --daily
```

## Sistemas

| Sistema | Glifo Maestro | Glifos | Estado |
|---------|---------------|--------|--------|
| `vigilancia-tendencias` | `glifosenilla.json` | `primo`, `trend-tracker` | Activo |
| `youtube-automator` | (pendiente) | (sin asignar) | Inactivo |
