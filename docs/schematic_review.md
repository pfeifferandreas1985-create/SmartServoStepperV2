# Hardware-Review: Smart Servo Stepper V2 Schaltplan (Phase 2)

**Review-Datum:** 18. Mai 2026  
**Geprüfte Datei:** `/home/ubuntu/Dokumente/output.pdf`  
**Status:** **Teilweise korrigiert – Nahezu produktionsbereit!**

---

## 1. Zusammenfassung der Änderungen (Was wurde korrigiert?)

Der aktualisierte Schaltplan wurde erfolgreich geprüft. Sie haben die kritischsten Stabilitätsprobleme hervorragend gelöst:

*   **TMC Enable Boot-Schutz (Erfolgreich gelöst):** `TMC_EN` (GPIO 3) wird jetzt über den **10 kΩ Widerstand R3** sauber gegen 3.3V gezogen. Der Motor bleibt beim Booten und Flashen absolut still.
*   **Split-UART Schutzwiderstand (Erfolgreich gelöst):** Der **1 kΩ Schutzwiderstand R5** wurde vom RX-Pfad in den TX-Pfad (`TMC_TX` an GPIO 6) verschoben. Damit ist die Kurzschlussgefahr bei Bus-Kollisionen vollständig gebannt!
*   **Symmetrische I2C Pull-Ups (Erfolgreich gelöst):** Sowohl `I2C_SDA` als auch `I2C_SCL` werden jetzt über symmetrische **4.7 kΩ Widerstände** (R2 und R1) versorgt. Das sorgt für saubere Flanken bei 400 kHz (I2C Fast Mode).

---

## 2. Antworten auf Ihre Fragen

### Frage 1: Soll ich SDA von GPIO 0 auf GPIO 20 legen?
> [!IMPORTANT]
> **Ja, absolut!** GPIO 0 ist ein sogenannter **Boot-Strapping-Pin**. Liegt SDA auf GPIO 0 und zieht der AS5600-Encoder die Leitung beim Einschalten auf GND, bootet der ESP32-C3 nicht mehr in Ihre Firmware, sondern bleibt im Download-Modus hängen. 
> 
> Wenn Sie **SDA auf GPIO 20** legen, ist Ihr I2C-Bus wie folgt verdrahtet:
> *   **I2C_SDA:** GPIO 20 (Pin 15) mit 4.7 kΩ Pull-Up gegen 3.3V.
> *   **I2C_SCL:** GPIO 10 (Pin 14) mit 4.7 kΩ Pull-Up gegen 3.3V.
> *   **GPIO 0:** Bleibt komplett unbeschaltet (perfekt für einen sicheren Boot-Vorgang).
> 
> *Hinweis:* Im offiziellen V1.7-Standard des Repositories ist es genau umgekehrt definiert (SDA = GPIO 10, SCL = GPIO 20). Da der ESP32-C3 die Pins jedoch völlig frei konfigurieren kann, ist Ihre Kombination (**SDA = GPIO 20, SCL = GPIO 10**) **technisch absolut gleichwertig und 100% sicher!**

---

### Frage 4: Muss DIAG angeschlossen sein oder kann man diese Funktionen über UART nutzen?
*   **Ja, man kann die StallGuard-Werte (Lasterkennung) über UART auslesen:** Der TMC2209 aktualisiert kontinuierlich das Register `SG_RESULT` (Wert `0` = Blockade). Ihre Firmware kann diesen Wert zyklisch über UART abfragen.
*   **Warum Sie DIAG trotzdem anschließen sollten (Empfehlung):**
    1.  **Latenz (Verzögerung):** Eine UART-Abfrage dauert ca. 1–2 Millisekunden. In dieser Zeit dreht der blockierte Motor weiter und rattert lautstark. Der `DIAG`-Pin ist ein **Hardware-Interrupt**. Er schaltet im selben Mikrosekundenbruchteil auf HIGH, in dem die Blockade auftritt. Der ESP32 kann dadurch sofort stoppen.
    2.  **Bibliotheks-Support:** Fast alle bekannten Bibliotheken (wie `TMCStepper`) nutzen den DIAG-Pin als externen Interrupt für das sensorlose Referenzieren (Sensorless Homing).
*   **Wann Sie DIAG weglassen können:**
    *   Da Sie den **AS5600 Magnet-Encoder** für echtes **Closed-Loop-Control** verbaut haben, benötigen Sie StallGuard für die Positionskontrolle meist gar nicht! Der Encoder erkennt ohnehin sofort jede Abweichung zwischen Soll- und Ist-Position. Wenn Sie kein sensorloses Homing verwenden, sondern normale Endschalter (oder rein über den Encoder referenzieren), kann der DIAG-Pin **problemlos unbeschaltet bleiben**.

---

## 3. Letzte verbleibende Detail-Empfehlungen (Feinschliff)

Falls Sie sich entscheiden, den **DIAG-Pin** für maximale Flexibilität doch anzuschließen, können Sie ein geniales Pin-Mapping-Dominospiel nutzen, das gleichzeitig Pins spart:

```
┌────────────────────────────────────────────────────────┐
│ UART-Pins zusammenführen (Single-Wire)                  │
│ ESP32 GPIO 6 ──► [ 1kΩ Schutz ] ──┬──► TMC2209 PDN     │
│                                   └──► 4.7kΩ Pull-Up   │
└────────────────────────────────────────────────────────┘
```

1.  **UART auf einen Pin reduzieren:** Da der ESP32-C3 hardwareseitig echten Eindraht-UART (Half-Duplex) unterstützt, können Sie `TMC_TX` und `TMC_RX` auf dem ESP32-Pin **GPIO 6** zusammenführen (über den 1 kΩ Serienwiderstand).
2.  **GPIO 5 freigeben:** Dadurch wird **GPIO 5** (der bisherige `TMC_RX`) frei!
3.  **TMC_DIR verschieben:** Legen Sie `TMC_DIR` von GPIO 2 auf den frei gewordenen **GPIO 5** (entspricht exakt der V1.7 Spezifikation).
4.  **GPIO 2 für DIAG nutzen:** Nun ist **GPIO 2** frei und Sie können den `DIAG`-Pin des TMC2209 dorthin verkabeln (mit einem 4.7 kΩ Pull-Up gegen 3.3V).

Mit diesem Kniff haben Sie **DIAG voll funktionsfähig angeschlossen**, ohne einen einzigen zusätzlichen Pin des ESP32-C3 Super Mini zu verschwenden!

---

## 4. Fazit der Freigabe

*   Wenn Sie **SDA auf GPIO 20** verschieben und die DIAG-Funktion nicht zwingend über einen Hardware-Interrupt benötigen: **Der Schaltplan ist hiermit FREIGEGEBEN für das PCB-Layout!**
*   Wenn Sie die DIAG-Interrupt-Funktion nutzen möchten: Setzen Sie das unter Abschnitt 3 beschriebene Pin-Dominospiel um. Danach ist der Plan ebenfalls **FREIGEGEBEN**.
