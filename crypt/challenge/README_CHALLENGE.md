# BDGB CRYPT CHALLENGE

> **"Masa Madre" — La rejilla binaria 16×16 como cifrado. Sin OpenSSL. Sin librerías. Sólo Kaprekar.**

---

## El Desafío

En `secret.bdgb` hay un mensaje cifrado con **BDGB Crypt v1**, un cifrado de flujo cuyo único fundamento matemático es la dinámica Kaprekar sobre una rejilla binaria de 256 nodos (16×16).

**Objetivo:** Extraer el mensaje original.

---

## Lo que tienes

| Elemento | Archivo |
|---|---|
| Archivo cifrado | [`secret.bdgb`](secret.bdgb) |
| Código fuente completo | [`crypt/bdgb_crypt.c`](../bdgb_crypt.c) |
| Cabecera | [`crypt/bdgb_crypt.h`](../bdgb_crypt.h) |
| Núcleo BDGB + tests | Repositorio completo en GitHub |

### Formato del archivo `.bdgb`

```
Offset  Contenido
──────────────────────────────────────
 0-3    Magic:  "BDGB" (4 bytes)
 4      Versión: 0x01 (1 byte)
 5-12   IV:      8 bytes aleatorios
 13+    Datos cifrados (XOR con keystream)
```

---

## Cómo descifrar (si tuvieras la contraseña)

```bash
git clone https://github.com/TU_USUARIO/bdgb.git
cd bdgb
mkdir build && cd build
cmake -G Ninja ..
ninja
./bdgb --decrypt crypt/challenge/secret.bdgb mensaje.txt "LA_CONTRASENA"
```

---

## Pistas estructurales

1. **La S-box** no es una tabla lookup estática: es `bdgb_sbox()`, que computa en tiempo real las propiedades geométricas (`densidad`, `radio`, `simetría`, `tipo_geom`) del nodo en la rejilla 16×16, combinadas con los pasos al atractor y el popcount.

2. **El estado interno** son 4 palabras de 32 bits (`uint32_t wseed[4]`). La derivación de clave procesa cada byte de la contraseña + IV mediante mezcla aritmética (XOR, suma, rotación) y la S-box geométrica.

3. **El keystream** se genera mezclando las 4 palabras de estado con retroalimentación del contador. Cada byte de keystream depende de los 4 words del estado actual.

4. **La rejilla 16×16** tiene 256 nodos, cada uno con un valor 8-bit (0-255). Las propiedades geométricas se derivan de la representación binaria de cada nodo.

5. **La contraseña es alfanumérica** (a-z, 0-9).

---

## Reglas

- No hay límite de tiempo.
- Se permite cualquier método: fuerza bruta, criptoanálisis, ingeniería inversa.
- El ganador debe enviar: (a) la contraseña, (b) el mensaje descifrado, (c) una explicación del método usado.
- Si se encuentra una vulnerabilidad en el diseño, se recompensará con crédito en el repositorio y mención en la documentación oficial.

---

## ¿Por qué este reto?

BDGB nació como un sistema de conocimiento geométrico para un estudio de pixel art. El cifrado es una consecuencia natural del modelo matemático subyacente.

Si **nadie** logra romperlo, validamos que la geometría binaria de Kaprekar puede servir como base para sistemas de protección ligeros, sin dependencias externas.

Si **alguien** lo rompe, descubriremos una debilidad que fortalecerá la "masa madre" antes de usarla en producción.

---

*Pixel Art Studio, 2026*
