[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/eoLoJBwi)
# TP3 - Ahorcado distribuido

## Compilación

```bash
mkdir -p build
cd build
cmake ..
make
```

Esto generará tres ejecutables:

-   `server`: El servidor del juego
-   `client`: El cliente con interfaz gráfica
-   `main`: La versión local del juego como demo

## Uso

### Ejecutar demo local

```bash
cd build
./main
```

### Ejecutar el Servidor

```bash
cd build
./server
```

### Conectar Clientes

En terminales separadas:

```bash
cd build
./client
```

## Testing

Para ejecutar los tests del juego:

```bash
cd build
./tp_tests
```
