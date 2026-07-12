# Captura de API NotebookLM

## Preparacion
1. Abre Chrome normal (NO la PWA shortcut)
2. Ve a `https://notebooklm.google.com` e inicia sesion
3. Abre DevTools: F12 → pestaña "Network" (Red)
4. Marca "Preserve log" (Preservar registro)
5. Filtra por "Fetch/XHR" para ver solo llamadas API

## Acciones a capturar (haz una por vez, en orden)

### 1. CREAR CUADERNO
- Haz click en "+ Crear nuevo cuaderno"
- En la red: busca una peticion POST o PUT con "notebook" en la URL
- Copia: URL completa, Request Headers (especialmente Authorization/cookies), Request Body, Response

### 2. SUBIR PDF
- Click en "Agregar fuente" o boton de subir archivo
- Selecciona un PDF cualquiera
- Busca peticion con "upload", "source", o "file" en la URL
- Copia: URL, Headers, Body (deberia ser multipart/form-data), Response

### 3. STUDIO + CREAR VIDEO
- Click en pestaña "Studio"
- Click en "Crear video"
- Click en "Crear" (confirmar)
- Busca peticiones con "video", "generate", "studio" en la URL
- Copia: URL, Headers, Body, Response

### 4. ELIMINAR CUADERNO (opcional pero util)
- Menu 3 puntos → Eliminar cuaderno
- Busca peticion DELETE o POST con "delete" o "notebook" en la URL
- Copia: URL, Headers, Body, Response

## Que copiar de CADA peticion
```
URL: https://...
Method: POST/GET/PUT/DELETE
Headers:
  - authorization: (si aparece)
  - content-type: 
  - cookie: (solo las primeras 20 caracteres)
  - x-*: (headers personalizados)
Body: (el JSON o form data que envia)
Response: (el JSON que devuelve)
```

Pasame cada una y construimos las llamadas directas.
