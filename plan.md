# Projekt: Strength Training PWA - GitHub Issues

Tento dokument obsahuje zoznam úloh (Issues) pripravených na automatické vytvorenie v GitHub repozitári. Každá úloha obsahuje názov, popis, štítky (labels) a odhadovanú prioritu.

---

## Issue 1: Inicializácia repozitára a základná štruktúra projektu (Frontend & Backend)
- **Labels:** `enhancement`, `setup`, `frontend`, `backend`
- **Priority:** High
- **Description:**
  Vytvoriť základnú štruktúru pre monorepo alebo oddelené zložky pre frontend (React.js s TypeScriptom) a backend (Node.js).
  
  **Úlohy:**
  - Inicializovať React aplikáciu pomocou Vite + TypeScript (frontend).
  - Inicializovať Node.js + Express server (backend).
  - Nastaviť `.gitignore` pre Node, Vite a environmentálne premenné.
  - Vytvoriť základnú štruktúru adresárov pre komponenty, štaty a API routes.

---

## Issue 2: Návrh a vytvorenie databázovej schémy pre používateľov a tréningy
- **Labels:** `database`, `backend`, `enhancement`
- **Priority:** High
- **Description:**
  Navrhnúť relačnú alebo dokumentovú databázu (napr. PostgreSQL / MongoDB) pre ukladanie používateľských profilov, tréningových plánov, cvičení a zaznamenaných sérií.
  
  **Úlohy:**
  - Navrhnúť schému pre `User`, `WorkoutSession`, `Exercise`, `Set` (váha, opakovania, RPE).
  - Nastaviť pripojenie k databáze na backendovej strane.
  - Vytvoriť základné migračné skripty alebo modely.

---

## Issue 3: Implementácia autentifikácie používateľov (JWT / Sessions)
- **Labels:** `security`, `backend`, `frontend`
- **Priority:** Medium
- **Description:**
  Zabezpečiť prístup k aplikácii pomocou bezpečnej autentifikácie, aby každý používateľ videl iba svoje tréningové dáta.
  
  **Úlohy:**
  - Vytvoriť API endpointy na registráciu a prihlásenie (`/api/auth/register`, `/api/auth/login`).
  - Implementovať hashovanie hesiel (napr. bcrypt).
  - Na fronte vytvoriť prihlasovací a registračný formulár a ukladanie tokenu (napr. HttpOnly cookies alebo secure storage).

---

## Issue 4: Integrácia Web Bluetooth API na pripojenie smart zariadení/senzorov
- **Labels:** `feature`, `frontend`, `hardware`
- **Priority:** Medium
- **Description:**
  Implementovať podporu Web Bluetooth API priamo v prehliadači na bezdrôtové pripojenie externých trénerských senzorov alebo zariadení (napr. inteligentné činky, snímače tepu/výkonu).
  
  **Úlohy:**
  - Vytvoriť servisný modul pre detekciu a párovanie Bluetooth zariadení v podporovaných prehliadačoch.
  - Ošetriť stavy nepodporovaných prehliadačov (fallback / chybové hlášky).
  - Zabezpečiť čítanie základných charakteristík zo zariadenia do stavu aplikácie.

---

## Issue 5: Používateľské rozhranie pre zaznamenávanie tréningu a výpočet RPE
- **Labels:** `frontend`, `ui/ux`, `feature`
- **Priority:** High
- **Description:**
  Vytvoriť intuitívne rozhranie pre aktívny tréning, kde si používateľ môže zaznamenávať jednotlivé série, váhy, opakovania a hodnotiť intenzitu pomocou RPE (Rate of Perceived Exertion).
  
  **Úlohy:**
  - Navrhnúť obrazovku aktívneho tréningu (pridanie cviku, série, úprava váhy/opakovaní).
  - Pridať komponent/slider pre výber RPE (hodnoty 6 až 10).
  - Prepojiť formulár s API na uloženie hotového tréningu do databázy.

---

## Issue 6: PWA konfiguračná vrstva (Service Workers & Offline režim)
- **Labels:** `pwa`, `frontend`, `performance`
- **Priority:** Medium
- **Description:**
  Premeniť webovú aplikáciu na plnohodnotnú PWA (Progressive Web App), aby fungovala aj pri nestabilnom internetovom pripojení v posilňovni.
  
  **Úlohy:**
  - Vytvoriť a nakonfigurovať `manifest.json` (ikony, názov, theme_color).
  - Implementovať Service Worker (napr. cez Workbox) pre cachovanie statických assetov.
  - Otestovať offline správanie aplikácie.

---

## Issue 7: Dashboard so štatistikami a vizualizáciou výkonov
- **Labels:** `frontend`, `analytics`, `feature`
- **Priority:** Low
- **Description:**
  Pridať prehľadnú sekciu so štatistikami a grafmi vývoja silových výkonov v čase (napr. progres na vybraných cvikoch).
  
  **Úlohy:**
  - Integrovať knižnicu na tvorbu grafov (n.apr. Chart.js / Recharts).
  - Vytvoriť endpointy na agregáciu údajov o maximálkach (Estimated 1RM) a objeme tréningu.
  - Vykresliť prehľadné grafy vývoja pre používateľa.