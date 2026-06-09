# Sistema de Glifos — Colmena y Panales

## Jerarquia

```
FAMILIA — grupo de colmenas que comparten un proposito superior
  ├── COLMENA — orquesta panales completos
  │     ├── PANAL — flujo de trabajo
  │     │     ├── GLIFO
  │     │     └── GLIFO
  │     └── PANAL
  └── COLMENA
```

Cuatro niveles:

| Nivel | Que es | Analogia |
|-------|--------|----------|
| **Familia** | Grupo de colmenas afines. Estructura superior total | El corporativo |
| **Colmena** | Orquesta panales completos como glifos | La fabrica |
| **Panal** | Flujo de trabajo con proposito definido | La linea de ensamblaje |
| **Glifo** | Nodo operativo que manipula informacion | La estacion de trabajo |

Un **Panal** puede actuar como **Glifo** dentro de otro Panal o de una Colmena. Esto se declara con `"tipo": "panal"` en la lista de glifos.

## Arquitectura

Un **Panal** es un **flujo de trabajo** con un proposito definido. Los glifos son los nodos operativos.

Todo panal tiene una **Semilla** (`semilla.json`) en su raiz que es la autoridad del panal. Define:

- Los glifos que lo componen
- Como se relacionan entre ellos
- El orden de ejecucion (pipeline)
- El **Tiempo**: glifo de control que determina cuando se activa el panal
- Si cada glifo es **actualizable** o no
- Datos extra propios de la tarea del panal

## Ramas de panales

Cada panal pertenece a una de tres ramas segun el origen de sus herramientas:

| Rama | Herramientas | Ejemplos |
|------|-------------|----------|
| **Online** | Servicios web, APIs externas | Google Trends, YouTube API, OpenAI, scraping web |
| **Local** | Corren 100% en el dispositivo | Scripts Python, binarios C, herramientas CLI del SO |
| **Hibrida** | Combinan online y local | Scraper web + procesamiento local + API cloud |

## Que hace un glifo

Un **Glifo manipula informacion usando herramientas como instrumentos** para cumplir el flujo de trabajo que se le encarga. Su logica es de orquestacion: decide que herramienta invocar, con que argumentos, en que orden, y como procesar el resultado para la siguiente etapa.

Las **herramientas** ejecutan las operaciones concretas; el glifo las orquesta para completar la tarea:

```
Glifo recibe: "vigilar tendencias hoy"
       ↓
  [GLIFO: invoca primo (RSS) → recibe reporte]
       ↓
  [GLIFO: pasa reporte a trend-tracker → recibe JSON+PDF]
       ↓
  [GLIFO: inyecta resultados en BDGB]
Glifo entrega: "tendencias del dia inyectadas"
```

El glifo no es un transportador pasivo: **manipula activamente el flujo de informacion**, usando cada herramienta como un instrumento para obtener el resultado que necesita. Es el chef, no la cuchara.

Esto hace que los glifos sean inherentemente **cross-platform**: la logica de orquestacion es portable, y las herramientas se adaptan al SO.

## El Tiempo: glifo de control

Cada panal tiene un **Tiempo**: el glifo de control que arranca primero y determina cuando se activa el panal. Hay dos tipos:

| Tipo | Descripcion | Ejemplo |
|------|-------------|---------|
| **Autonomo** | Se activa por horario propio. Funciona sin intervencion externa | Monitoreo diario de tendencias a las 08:00 |
| **Dirigido** | Lo activa un agente externo: usuario, programa, o señal de otro panal | Un panal que se ejecuta cuando el usuario abre un programa |

El Tiempo se define en la semilla dentro del bloque `"tiempo"`. Cada panal tiene un Tiempo distinto segun su naturaleza.

## Actualizacion de glifos

Cada glifo declara explicitamente si es **actualizable** (`"actualizable": true/false`) en la semilla:

- `actualizable: false` — El glifo es fijo, no puede modificarse sin cambiar la semilla
- `actualizable: true` — El glifo puede recibir actualizaciones de su codigo o configuracion sin alterar la semilla

Esto permite tener glifos externos (scripts Python) que se puedan actualizar independientemente, mientras que los glifos nativos (C compilado) son fijos.

## Independencia y composicion

Cada panal funciona **independiente** de otros panales. Pero un panal completo puede integrarse como un **glifo** dentro de otro panal o dentro de una **colmena**:

```
COLMEÑA (colmena-de-contenido)
  ├── PANAL: vigilancia-tendencias ← integrado como glifo
  │     ├── glifo: primo
  │     └── glifo: trend-tracker
  ├── PANAL: youtube-automator ← integrado como glifo
  │     └── ...
  └── glifo: publicador (nativo)
```

Esto se define en `glifos[]` con `"tipo": "panal"`:

```json
{
  "id": "vigilancia-tendencias",
  "tipo": "panal",
  "ref": "glifos/vigilancia-tendencias/semilla.json",
  "descripcion": "Panal completo integrado como glifo"
}
```

Una **Colmena** es un nivel superior que no tiene glifos propios: solo orquesta panales completos. Su semilla define que panales la componen y como se relacionan entre ellos.

Una **Familia** es el nivel maximo: agrupa colmenas que comparten un proposito superior. No tiene pipeline propio, solo define que colmenas la integran. La familia no necesita semilla: es puramente organizativa, se define en el registry.

## Estructura de directorios

```
glifos/
  README.md
  registry.json
  familias/                          ← familias (opcional)
    familia-ejemplo/                 ← agrupa colmenas afines
      registry.json
  colmenas/                          ← colmenas
    colmena-ejemplo/
      semilla.json                   ← SEMILLA de la colmena
  vigilancia-tendencias/             ← panal
    semilla.json                     ← SEMILLA del panal
    README.md
    glifos/
      primo/glifo.json               ← config del glifo nativo
      trend-tracker/                 ← codigo del glifo externo
        glifo.json
        trend_tracker.py
        daily/ + weekly/
  youtube-automator/                 ← panal sin semilla aun
    config.json
```

## Esquema de semilla.json

Cada panal define su `semilla.json` con esta estructura:

| Campo | Tipo | Descripcion |
|-------|------|-------------|
| `id` | string | Identificador unico del panal |
| `nombre` | string | Nombre humano |
| `tipo` | string | Siempre `"panal"` |
| `rama` | string | `online` / `local` / `hibrida` |
| `descripcion` | string | Que hace el panal |
| `estado` | string | `activo` / `inactivo` |
| `version` | string | Version del schema |
| `tiempo` | object | Glifo de control: tipo (autonomo/dirigido) + schedule |
| `glifos` | array | Lista de glifos del panal |
| `relaciones` | array | Conexiones entre glifos (de, a, tipo, flujo) |
| `pipeline` | object | Orden de ejecucion con pasos y dependencias |
| `datos_panal` | object | **Datos personalizados** para la tarea del panal |
| `metricas` | object | Contadores de ejecucion |

### Campo `tiempo`

Define el control de activacion del panal:

```json
{
  "tipo": "autonomo | dirigido",
  "schedule": {
    "tipo": "diario | semanal | mensual | una_vez",
    "hora": "HH:MM",
    "zona_horaria": "America/Mexico_City"
  }
}
```

### Campo `glifos[]`

Cada glifo tiene:

| Campo | Descripcion |
|-------|-------------|
| `id` | Identificador unico en el panal |
| `nombre` | Nombre humano |
| `tipo` | `nativo` (C), `externo` (Python, etc.), o `panal` (otro panal completo) |
| `entry` | Comando para ejecutarlo |
| `actualizable` | `true`/`false` — si puede actualizarse sin cambiar la semilla |
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

### Campo `datos_panal` (modificable)

Este campo es **personalizable segun la tarea del panal**. Cada panal define aqui los datos que necesita:
- Fuentes de datos, API keys, keywords, horarios, formatos, rutas, etc.
- No hay esquema fijo; cada panal adapta este objeto a su proposito

## Cross-platform

La logica de orquestacion del glifo funciona identico en Windows, Linux, macOS, Android, iOS, tablets y cualquier sistema con un sistema de archivos. El nucleo C compila con C99 en cualquier plataforma. Las herramientas externas (Python, etc.) pueden requerir runtimes especificos, pero el glifo que las orquesta no.

## Regla fundamental

**Ningun glifo funciona fuera de un panal.** Aunque se pueda crear por separado (su carpeta y codigo existen), no se ejecuta hasta que se define dentro de una `semilla.json`.

Un glifo sin panal es codigo muerto.

## Orden de creacion

1. **Definir el Panal** — crear carpeta con `semilla.json` (tiempo, glifos, relaciones, pipeline, datos_panal)
2. **Crear los Glifos** — cada uno en `panal/glifos/<id>/` con su codigo

La autoridad es `semilla.json`. Si un glifo no esta ahi declarado, no existe para el panal.

## Clasificacion por naturaleza

| Clase | Que es | Ejemplo |
|-------|--------|---------|
| **Primo** | Codigo original del que se crean todos los demas glifos. Es el molde, la masa madre | `src/glifo.c`, `include/glifo.h` |
| **Semilla** | `semilla.json`. Eje que define y crea un panal. Sin semilla no hay panal | `vigilancia-tendencias/semilla.json` |
| **Comun** | Glifos operativos que ejecutan las tareas dentro de un panal | `primo` (como glifo comun), `trend-tracker` |

### Subtipos de glifo comun

| Subtipo | Descripcion |
|---------|-------------|
| **Nativo** | Compilado en el binario BDGB (`src/glifo.c`). Cero dependencias. No actualizable |
| **Externo** | Script (Python, etc.) dentro de la carpeta del panal. Puede ser actualizable |

## Comandos

```bash
bdgb --glifo-list                    # solo lista glifos dentro de panales activos
bdgb --glifo-run primo               # busca primo en semilla.json del panal
python3 glifos/vigilancia-tendencias/glifos/trend-tracker/trend_tracker.py --daily
```

## Panales

| Panal | Semilla | Tiempo | Glifos | Rama | Estado |
|-------|---------|--------|--------|------|--------|
| `vigilancia-tendencias` | `semilla.json` | Autonomo (diario 08:00) | `primo`, `trend-tracker` | online | Activo |
| `youtube-automator` | (pendiente) | (pendiente) | (sin asignar) | — | Inactivo |

## Colmenas

| Colmena | Semilla | Panales | Estado |
|---------|---------|---------|--------|
| (aun no definida) | — | — | — |

## Familias

| Familia | Colmenas | Proposito |
|---------|----------|-----------|
| (aun no definida) | — | — |
