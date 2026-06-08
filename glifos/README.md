# Sistema de Glifos

## Arquitectura

Cada **Glifo** pertenece a un **Sistema**. Un Sistema es un **flujo de trabajo** con un proposito definido. Los glifos son los nodos operativos.

Todo sistema tiene un **glifo maestro** (`glifosenilla.json`) en su raiz que define:

- Los glifos que lo componen
- Como se relacionan entre ellos
- El orden de ejecucion (pipeline)
- Datos extra propios de la tarea del sistema

```
glifos/
  README.md
  registry.json
  vigilancia-tendencias/         ← sistema
    glifosenilla.json            ← MAESTRO: glifos, relaciones, pipeline, datos_sistema
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

## Esquema de glifosenilla.json

Cada sistema define su `glifosenilla.json` con esta estructura:

| Campo | Tipo | Descripcion |
|-------|------|-------------|
| `id` | string | Identificador unico del sistema |
| `nombre` | string | Nombre humano |
| `tipo` | string | Siempre `"sistema"` |
| `descripcion` | string | Que hace el sistema |
| `estado` | string | `activo` / `inactivo` |
| `version` | string | Version del schema |
| `glifos` | array | Lista de glifos que componen el sistema |
| `relaciones` | array | Conexiones entre glifos (de, a, tipo, flujo) |
| `pipeline` | array | Orden de ejecucion con dependencias |
| `datos_sistema` | object | **Datos personalizados** para la tarea del sistema |
| `metricas` | object | Contadores de ejecucion |

### Campo `glifos[]`

Cada glifo tiene:

| Campo | Descripcion |
|-------|-------------|
| `id` | Identificador unico en el sistema |
| `nombre` | Nombre humano |
| `tipo` | `nativo` (C) o `externo` (Python, etc.) |
| `entry` | Comando para ejecutarlo |
| `descripcion` | Que hace |

### Campo `relaciones[]`

Define como se conectan los glifos:

```json
{
  "de": "primo",
  "a": "trend-tracker",
  "tipo": "ejecucion",
  "flujo": "descripcion de como se relacionan"
}
```

### Campo `datos_sistema` (modificable)

Este campo es **personalizable segun la tarea del sistema**. Cada sistema define aqui los datos que necesita:
- Fuentes de datos, API keys, keywords, horarios, formatos, rutas, etc.
- No hay esquema fijo; cada sistema adapta este objeto a su proposito

## Orden de creacion

1. **Definir el Sistema** — crear carpeta con `glifosenilla.json` (glifos, relaciones, pipeline, datos_sistema)
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
