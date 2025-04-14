# Projekt JIMP2 c

# Przykładowy format grafu do użycia
```
5 6
2 3
1 3 4
1 2 4 5
2 3 5
3 4
```
Pierwsza linia:

- `5` - Liczba wierzchołków (węzłów) w grafie.
- `6` - Liczba krawędzi w grafie.

Kolejne linie:

- Każda linia odpowiada jednemu wierzchołkowi (licząc od wierzchołka `1`).
- Liczby w każdej linii reprezentują sąsiadów danego wierzchołka.

Na przykład:
- Linia `2`: Wierzchołek `1` jest połączony z wierzchołkami `2` i `3`.
- Linia `3`: Wierzchołek `2` jest połączony z wierzchołkami `1`, `3` i `4`.

Uwagi:

- Wierzchołki są numerowane od `1`.
- Graf jest nieskierowany, co oznacza, że jeśli wierzchołek `1` jest połączony z wierzchołkiem `2`, to wierzchołek `2` jest również połączony z wierzchołkiem `1`. Jednak każda krawędź jest zapisywana tylko raz (bez duplikacji).
