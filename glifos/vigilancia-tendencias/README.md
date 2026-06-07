# Sistema: Vigilancia de Tendencias

Monitoreo diario de Google Trends, generacion de reportes y alimentacion del nucleo BDGB.

## Glifos

| Glifo | Tipo | Proposito |
|-------|------|-----------|
| `primo` | Nativo (C) | Fetch basico de tendencias, reporte .txt, inyeccion rapida |
| `trend-tracker` | Externo (Python) | Reportes JSON/PDF, resumen semanal, exportacion |

## Flujo

```
primo (08:00) ──> reporte diario .txt
                        │
trend-tracker (08:05) ──> reporte JSON + PDF
                        │
                        └──> cada 7 dias -> resumen semanal
```

## Archivos

- `workflow.json` — define el pipeline y dependencias entre glifos
- `glifos/<id>/glifo.json` — configuracion individual de cada glifo
