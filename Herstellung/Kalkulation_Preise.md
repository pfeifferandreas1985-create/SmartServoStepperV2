# SmartServoStepperV2 – Herstellungskosten & Preiskalkulation

Dieses Dokument bietet eine vollständige Übersicht über die Herstellungskosten und das Projekt-Budget für den **SmartServoStepperV2** in verschiedenen Produktions-Staffelungen (10x, 100x, 1000x) sowie einen Vergleich zwischen dem Bezug aus China und Deutschland.

---

## 1. Einzelkomponenten-Preise (Übersicht pro Stück)

Hier siehst du, wie sich der Stückpreis der einzelnen Komponenten je nach Bestellmenge und Einkaufsquelle verändert.

### Bezug aus China (Aliexpress / LCSC / JLCPCB) – Preise pro Stück

| Komponente | Stückpreis bei 10x | Stückpreis bei 100x | Stückpreis bei 1000x |
| :--- | :--- | :--- | :--- |
| **ESP32-C3 Super Mini** | ~2,50 € | ~1,80 € | ~1,30 € |
| **Tenstar TMC2209 V2** | ~2,20 € | ~1,60 € | ~1,10 € |
| **MP1584EN Buck Modul** | ~0,80 € | ~0,50 € | ~0,35 € |
| **BSS138 Level Shifter Mosfets** | ~0,10 € | ~0,03 € | ~0,01 € |
| **Passiva & Schutz** (Kondensatoren, TVS, Widerstände, Stecker) | ~0,60 € (Set) | ~0,25 € | ~0,08 € |
| **Leiterplatte (PCB)** (z. B. JLCPCB) | ~1,50 € | ~0,40 € | ~0,15 € |
| **Gesamtkosten pro Stück (China)** | **~7,70 €** | **~4,58 €** | **~2,99 €** |

### Bezug aus Deutschland (Amazon / Reichelt / Conrad) – Preise pro Stück

| Komponente | Stückpreis bei 10x | Stückpreis bei 100x | Stückpreis bei 1000x |
| :--- | :--- | :--- | :--- |
| **ESP32-C3 Super Mini** | ~6,00 € | ~4,50 € | ~3,50 € |
| **Tenstar TMC2209 V2** | ~5,50 € | ~4,00 € | ~3,00 € |
| **MP1584EN Buck Modul** | ~2,50 € | ~1,80 € | ~1,20 € |
| **BSS138 Level Shifter Mosfets** | ~0,40 € | ~0,15 € | ~0,08 € |
| **Passiva & Schutz** (Kondensatoren, TVS, Widerstände, Stecker) | ~1,50 € (Set) | ~0,60 € | ~0,25 € |
| **Leiterplatte (PCB)** (DE-Express/EU) | ~4,50 € | ~1,50 € | ~0,75 € |
| **Gesamtkosten pro Stück (DE)** | **~20,40 €** | **~12,55 €** | **~8,78 €** |

---

## 2. Gesamtkosten nach Projekt-Staffelung (Projekt-Budget)

Hier sind die hochgerechneten Gesamtkosten für die jeweilige Gesamtbestellung (reine Materialkosten inkl. geschätztem Versand).

### 📊 Staffelvergleich im Überblick

| Staffel | Bezug China (Aliexpress/PCBA) | Bezug Deutschland (Amazon/Distributoren) | Hauptfokus / Charakteristik |
| :--- | :--- | :--- | :--- |
| **10x** | **~85 € bis 100 €** | **~220 € bis 250 €** | Prototyping & Entwicklung |
| **100x** | **~450 € bis 500 €** | **~1.250 € bis 1.400 €** | Kleinserie & Vorserie |
| **1000x** | **~2.800 € bis 3.200 €** | **~8.500 € bis 9.500 €** | Industrielle Produktion (PCBA) |

---

### 🔹 Staffelung 10x (Kleinserie / Prototyping)

*   **Bezug China (Aliexpress):** **~85 € bis 100 €**
    > *Hinweis:* PCBs kosten in China für 5–10 Stück pauschal nur ca. 2–5 $ (zzgl. Versand). Die Bauteile werden in Mindestmengen (oft 5er- oder 10er-Packs) gekauft.
*   **Bezug Deutschland (Amazon/Reichelt):** **~220 € bis 250 €**
    > *Hinweis:* Auf Amazon zahlt man für Modul-Mehrfachpacks (z. B. 3er-Pack ESP32) deutlich mehr, hat es aber in 24 Stunden da.

### 🔹 Staffelung 100x (Kleinserie)

*   **Bezug China (Aliexpress/LCSC):** **~450 € bis 500 €**
    > *Hinweis:* Hier greifen bereits echte Rabatte. Die PCBs kosten kaum noch etwas im Verhältnis. Der Versand fällt kaum ins Gewicht.
*   **Bezug Deutschland (Amazon/Distributoren):** **~1.250 € bis 1.400 €**
    > *Hinweis:* Amazon ist hierfür der falsche Ort (zu teuer). Man müsste über gewerbliche Distributoren (wie Bürklin, Rutronik oder Mouser DE) einkaufen, um diese Preise zu halten.

### 🔹 Staffelung 1000x (Industrielle Menge)

*   **Bezug China (Großhandel/Direct Sourcing):** **~2.800 € bis 3.200 €**
    > *Hinweis:* Bei dieser Menge kauft man keine "Module" mehr auf Aliexpress, sondern lässt die Platinen direkt in China (z. B. bei JLCPCB oder PCBWay) vollbestückt (PCBA) herstellen. Das ist oft günstiger, als die Module einzeln per Hand aufzulöten.
*   **Bezug Deutschland (Großhandel):** **~8.500 € bis 9.500 €**
    > *Hinweis:* Einkauf über offizielle deutsche Distributoren mit Mengenrabattstaffel und ggf. Zollabwicklung inklusive.

---

## 3. Wichtige Insider-Tipps für deine Kalkulation

> [!TIP]
> **Der PCBA-Faktor (Bestückung in China)**
> Da du Standardmodule (TMC2209 und ESP32) verwendest, schau dir bei einer 100er- oder 1000er-Staffel die Bestückungskosten (PCBA) von chinesischen Werken an. Es ist oft billiger, wenn das Werk die SMD-Kleinteile (Widerstände, Kondensatoren, TVS-Diode, Level Shifter) direkt maschinell auflötet. Du musst dann nur noch die zwei großen Module per Hand einstecken/einlöten.

> [!IMPORTANT]
> **Zoll & Einfuhrumsatzsteuer**
> Bei Bezug aus China fallen ab 150 € Warenwert Zollgebühren an (Einfuhrumsatzsteuer von 19 % wird bei Aliexpress meist direkt an der Kasse berechnet). Das ist in den obigen Gesamtkosten grob einkalkuliert.

> [!WARNING]
> **Qualitätsrisiko bei China-Modulen**
> Bei 1000 Stück TMC2209-Modulen aus extrem billigen China-Quellen musst du mit einer Ausschussrate von ca. **2–5 %** rechnen (Treiber, die defekt ankommen oder beim ersten Einschalten sterben). Plane dies unbedingt als Puffer ein.
