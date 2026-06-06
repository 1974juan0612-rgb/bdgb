# BDGB CRYPT CHALLENGE — Paquete para AI local

## Archivos incluidos

| Archivo | Propósito |
|---|---|
| `../bdgb_crypt.c` | Implementación completa del cifrado BDGB |
| `../bdgb_crypt.h` | Cabecera con tipos y API |
| `secret.bdgb` | Archivo cifrado (445 bytes) → extraer mensaje |
| `README_CHALLENGE.md` | Reglas y pistas para la comunidad |

## API pública

```c
int  bdgb_crypt_init(BDGBCryptCtx *ctx, const char *password, const uint8_t *iv);
void bdgb_crypt_iv_generate(uint8_t *iv);
void bdgb_crypt_keystream(BDGBCryptCtx *ctx, uint8_t *out, size_t len);
void bdgb_crypt_buffer(BDGBCryptCtx *ctx, uint8_t *data, size_t len);
int  bdgb_encrypt_file(const char *in_path, const char *out_path, const char *password);
int  bdgb_decrypt_file(const char *in_path, const char *out_path, const char *password);
```

## Estructura del cifrado

- **Estado:** 4 × `uint32_t wseed[4]` (32 bits cada uno)
- **S-box:** `bdgb_sbox()` — propiedades geométricas del nodo en rejilla 16×16
- **Derivación de clave:** mezcla aritmética (XOR, suma, rotación) de password + IV
- **Keystream:** mezcla de los 4 words con retroalimentación del contador

## Formato del archivo `.bdgb`

```
Offset 0-3:   "BDGB" (magic)
Offset 4:     0x01 (version)
Offset 5-12:  IV (8 bytes aleatorios)
Offset 13+:   ciphertext (plaintext XOR keystream)
```

## Hint

La contraseña es alfanumérica (a-z, 0-9). La S-box depende de la rejilla 16×16 — no es una tabla lookup fija, se computa en tiempo real.
