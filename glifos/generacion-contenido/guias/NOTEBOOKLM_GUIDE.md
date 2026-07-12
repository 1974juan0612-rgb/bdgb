# Guia de NotebookLM para Automatizacion PyAutoGUI

## Layout verificado por el usuario

### Pagina Principal (sin cuadernos)

```
+--------------------------------------------------+
| [?] [+ Crear nuevo cuaderno]          [avatar]    |
|                                                    |
|                "Crea tu primer cuaderno"           |
|                                                    |
|             ... contenido del medio ...            |
|                                                    |
+--------------------------------------------------+
```

### Boton "Crear nuevo cuaderno"
- **Posicion**: zona superior izquierda (no pegado a la esquina)
- **Ancho**: mas ancho que alto (tipo barra/boton ancho)
- **Fondo**: RGB ~12,12,12 (casi negro)
- **Texto**: blanco, con icono "+" antes del texto
- **Unico elemento oscuro** en la pagina cuando no hay cuadernos

### Toolbar superior
- Izquierda: boton "+ Crear nuevo cuaderno"
- Derecha: avatar de usuario, posiblemente otros iconos

## Flujo de trabajo

1. Crear cuaderno → click en "+ Crear nuevo cuaderno"
2. Subir PDF → click en "Agregar fuente" o boton de upload
3. Ir a Studio → click en pestana "Studio"
4. Crear video → click en "Crear video" → confirmar "Crear"
5. Descargar video → esperar generacion, descargar
6. Cerrar cuaderno → menu 3 puntos → "Eliminar cuaderno" → confirmar
