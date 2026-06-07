# Sistema de Glifos

## Arquitectura

Cada **Glifo** pertenece a un **Sistema**. Un Sistema es, en esencia, un **flujo de trabajo** con un proposito definido. Los glifos son los nodos operativos que ejecutan las tareas dentro de ese flujo.

```
SISTEMA (flujo de trabajo)
  ├── Glifo A  (tarea 1)
  ├── Glifo B  (tarea 2)
  └── Glifo C  (tarea 3)
```

## Orden de creacion

Lo correcto es:

1. **Definir el Sistema** — ?Que flujo de trabajo quiero automatizar? Ej: "Vigilancia de Tendencias", "Publicacion Automatica", "Analisis de Datos"
2. **Crear los Glifos** — ?Que tareas atomicas necesita ese flujo? Cada tarea es un glifo

Los glifos pueden crearse por separado y despues asignarse a un sistema, pero la arquitectura esta pensada para que el sistema se defina primero.

## Registro

El archivo `registry.json` contiene la definicion de todos los sistemas y sus glifos. Cada entrada tiene:

```json
{
  "sistema": "nombre-del-sistema",
  "descripcion": "Flujo de trabajo que automatiza",
  "glifos": [
    {
      "id": "identificador-unico",
      "nombre": "Nombre del Glifo",
      "tipo": "nativo | externo",
      "schedule": "diario | semanal | manual",
      "dependencias": ["glifo-id-1", "glifo-id-2"]
    }
  ]
}
```

## Tipos de Glifo

| Tipo | Descripcion |
|------|-------------|
| **Nativo** | Compilado en el binario BDGB (`src/glifo.c`). Ejecucion rapida, cero dependencias externas |
| **Externo** | Script en Python u otro lenguaje en `glifos/<id>/`. Ejecucion via `system()` o subproceso |

El **Glifo Primo** (`id: "primo"`) es el glifo nativo fundacional: Trend Tracker. Puede ejecutarse con:

```bash
bdgb --glifo-run primo
```

## Sistemas definidos actualmente

| Sistema | Glifos | Estado |
|---------|--------|--------|
| _(pendiente de definir)_ | `primo` (nativo) | Activo |
| _(pendiente de definir)_ | `trend-tracker` (externo) | Activo |
| _(pendiente de definir)_ | `youtube-automator` (externo) | Inactivo |

## Ciclo de vida de un Sistema

1. **Definicion** — Se crea la entrada en `registry.json` con el flujo de trabajo
2. **Implementacion** — Se crean los glifos necesarios (nativos en C o externos en Python)
3. **Registro** — Cada glifo se asocia al sistema via `registry.json`
4. **Ejecucion** — El supervisor del BDGB ejecuta los glifos segun su schedule
5. **Retroalimentacion** — Los resultados se guardan en BDGB (conceptos, NLP, reportes)

## Proximo sistema recomendado

Antes de crear mas glifos, define el primer sistema. Ejemplo:

```json
{
  "sistema": "vigilancia-tendencias",
  "descripcion": "Monitoreo diario de Google Trends, generacion de reportes y alimentacion del nucleo BDGB",
  "glifos": ["primo", "trend-tracker"]
}
```

Esto agrupa el glifo nativo `primo` y el externo `trend-tracker` bajo un mismo proposito.
