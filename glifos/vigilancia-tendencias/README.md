# Sistema: Vigilancia de Tendencias

Monitoreo diario de Google Trends, generacion de reportes y alimentacion del nucleo BDGB.

## Regla

Ningun glifo funciona fuera de un panal. Solo los glifos declarados en `semilla.json` existen y pueden ejecutarse.

## Glifo Maestro

`semilla.json` — define pipeline, Tiempo, relaciones y configuracion del panal.

## Glifos

| Glifo | Tipo | Produce |
|-------|------|---------|
| `primo` | Nativo (C) | reporte .txt, conceptos BDGB, terminos NLP |
| `trend-tracker` | Externo (Python) | reporte JSON, PDF, resumen semanal |

## Pipeline

```
08:00  primo ──> .txt + conceptos BDGB
08:05  trend-tracker ──> .json + .pdf
                        └──> cada 7 dias -> resumen semanal
```
