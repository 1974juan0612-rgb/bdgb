# Sistema de Glifos

## Arquitectura

Cada **Glifo** pertenece a un **Sistema**. Un Sistema es un **flujo de trabajo** con un proposito definido. Los glifos son los nodos operativos.

Los sistemas viven en su propia carpeta raiz:

```
glifos/
  README.md
  registry.json
  vigilancia-tendencias/         ← sistema
    README.md
    workflow.json                ← pipeline del sistema
    glifos/
      primo/glifo.json           ← config del glifo nativo
      trend-tracker/             ← codigo del glifo externo
        glifo.json
        trend_tracker.py
        daily/
        weekly/
  youtube-automator/             ← sistema pendiente de definir
    config.json
```

## Orden de creacion

1. **Definir el Sistema** — ?Que flujo de trabajo?
2. **Crear los Glifos** — ?Que tareas atomicas?

## Tipos de Glifo

| Tipo | Descripcion |
|------|-------------|
| **Nativo** | Compilado en el binario BDGB (`src/glifo.c`). Cero dependencias |
| **Externo** | Script (Python, etc.) dentro de la carpeta del sistema |

## Comandos

```bash
bdgb --glifo-list            # lista todos los glifos disponibles
bdgb --glifo-run primo       # ejecuta glifo nativo
python3 glifos/vigilancia-tendencias/glifos/trend-tracker/trend_tracker.py --daily
```

## Sistemas definidos

| Sistema | Glifos | Estado |
|---------|--------|--------|
| `vigilancia-tendencias` | `primo`, `trend-tracker` | Activo |
| `youtube-automator` | (sin sistema asignado) | Inactivo |
