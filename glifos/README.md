# Sistema de Glifos

## Arquitectura

Cada **Glifo** pertenece a un **Sistema**. Un Sistema es un **flujo de trabajo** con un proposito definido. Los glifos son los nodos operativos.

Todo sistema tiene un **glifo maestro** (`glifosenilla.json`) en su raiz que define:

- Los glifos que lo componen
- Como se relacionan entre ellos
- El orden de ejecucion (pipeline)
- Datos extra propios de la tarea del sistema

## Independencia y composicion

Cada sistema funciona **independiente** de otros sistemas. Pero un sistema completo puede integrarse como un **glifo** dentro de un sistema superior:

```
SISTEMA PADRE (produccion-automatica)
  ├── glifo: scraper (nativo)
  ├── glifo: editor (externo)
  └── SISTEMA: vigilancia-tendencias ← integrado como glifo
        ├── glifo: primo
        └── glifo: trend-tracker
```

Esto se define en `glifos[]` con `"tipo": "sistema"`:

```json
{
  "id": "vigilancia-tendencias",
  "tipo": "sistema",
  "ref": "glifos/vigilancia-tendencias/glifosenilla.json",
  "descripcion": "Sistema completo integrado como glifo"
}
```

## Estructura de directorios

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

## Regla fundamental

**Ningun glifo funciona fuera de un sistema.** Aunque se pueda crear por separado (su carpeta y codigo existen), no se ejecuta hasta que se define dentro de un `glifosenilla.json`.

Un glifo sin sistema es codigo muerto.

## Orden de creacion

1. **Definir el Sistema** — crear carpeta con `glifosenilla.json` (glifos, relaciones, pipeline, datos_sistema)
2. **Crear los Glifos** — cada uno en `sistema/glifos/<id>/` con su codigo

La autoridad es `glifosenilla.json`. Si un glifo no esta ahi declarado, no existe para el sistema.

## Clasificacion por naturaleza

| Clase | Que es | Ejemplo |
|-------|--------|---------|
| **Primo** | Codigo original del que se crean todos los demas glifos. Es el molde, la masa madre | `src/glifo.c`, `include/glifo.h` |
| **Semilla** | `glifosenilla.json`. Eje que define y crea un sistema. Sin semilla no hay sistema | `vigilancia-tendencias/glifosenilla.json` |
| **Comun** | Glifos operativos que ejecutan las tareas dentro de un sistema | `primo` (como glifo comun), `trend-tracker` |

### Subtipos de glifo comun

| Subtipo | Descripcion |
|---------|-------------|
| **Nativo** | Compilado en el binario BDGB (`src/glifo.c`). Cero dependencias |
| **Externo** | Script (Python, etc.) dentro de la carpeta del sistema |

## Comandos

```bash
bdgb --glifo-list                    # solo lista glifos dentro de sistemas activos
bdgb --glifo-run primo               # busca primo en glifosenilla.json del sistema
python3 glifos/vigilancia-tendencias/glifos/trend-tracker/trend_tracker.py --daily
```

## Sistemas

| Sistema | Glifo Maestro | Glifos | Estado |
|---------|---------------|--------|--------|
| `vigilancia-tendencias` | `glifosenilla.json` | `primo`, `trend-tracker` | Activo |
| `youtube-automator` | (pendiente) | (sin asignar - no funciona) | Inactivo |
