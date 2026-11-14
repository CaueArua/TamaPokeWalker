#include "help_screen.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../i18n.h"
#include "../../audio.h"
#include "clock_screen.h"

#define HELP_PAGE_COUNT 8
#define HELP_LINE_COUNT 6

extern Arduino_Canvas *gfx;
extern uint8_t helpPage;
extern bool helpOpen;
extern bool clockOpen;
extern bool helpDirty;
extern bool clockDirty;
extern void flushStampedFrame();

HelpScreen helpScreen;
static constexpr int SCREEN_CX = 233;
static const char *const HELP_WORD[LANG_COUNT] = { "AYUDA", "HELP", "AIDE", "HILFE", "AIUTO", "AJUDA" };
static const char *const HELP_OK[LANG_COUNT] = { "OK", "OK", "OK", "OK", "OK", "OK" };

static const char *const HELP_TITLES[LANG_COUNT][HELP_PAGE_COUNT] = {
  { "CUIDADO", "SUENO/ENERGIA", "MINIJUEGOS", "COMBATE 1", "COMBATE 2", "COLECCION", "EXTRAS", "EXPEDICION" },
  { "CARE", "SLEEP/ENERGY", "MINIGAMES", "BATTLE 1", "BATTLE 2", "COLLECTION", "EXTRAS", "EXPEDITION" },
  { "SOIN", "SOMMEIL/ENE", "MINI-JEUX", "COMBAT 1", "COMBAT 2", "COLLECTION", "EXTRAS", "EXPEDITION" },
  { "PFLEGE", "SCHLAF/ENERGIE", "MINISPIELE", "KAMPF 1", "KAMPF 2", "SAMMLUNG", "EXTRAS", "EXPEDITION" },
  { "CURA", "SONNO/ENERGIA", "MINIGIOCHI", "LOTTA 1", "LOTTA 2", "COLLEZIONE", "EXTRA", "SPEDIZIONE" },
  { "CUIDADO", "SONO/ENERGIA", "MINIJOGOS", "BATALHA 1", "BATALHA 2", "COLECAO", "EXTRAS", "EXPEDICAO" },
};

static const char *const HELP_LINES[LANG_COUNT][HELP_PAGE_COUNT][HELP_LINE_COUNT] = {
  {
    { "Comida baja = descuido.", "Jugar sube alegria.", "Bano limpia suciedad.", "Tocar da alegria/vinc.", "Peso alto te frena.", "Dulce alegra, engorda." },
    { "Dormir recupera energia.", "Durmiendo todo baja lento.", "Luz despierta o duerme.", "PWR corto apaga pantalla.", "Ahorro usa light sleep.", "Sin borrar conserva save." },
    { "Bola: toca la bola.", "Atrapa: toca iconos.", "Memo: repite secuencia.", "Limpia: toca manchas.", "Tipo: elige ventaja.", "Dan records y entreno." },
    { "Rapido: menos dano.", "Rival esquiva poco.", "Recibes algo menos dano.", "Fuerte: mas dano.", "Riesgo y contra mayor.", "No siempre conviene." },
    { "Esquivar evita dano.", "Si sale: Contra listo.", "Prox ataque pega mas.", "Ruhe/Descanso cura 2x.", "Tambien da Guardia.", "Tipos suben/bajan dano." },
    { "Pokedex: desliza lado.", "Criado y atrapado cuentan.", "10/25/50/100/151/160/200/251/300/386: marcos.", "Perfil: elige marco.", "Detalle conocido: chirp.", "SON TODO: toca pet." },
    { "Diario da metas diarias.", "Eventos salen raros.", "Batallas salvajes opc.", "Captura tras ganar.", "Rachas y medallas quedan.", "Agita: juega. Paseo da FEL." },
    { "Expedicion: 15/30/60 min.", "Cuesta energia al salir.", "El bicho sigue disponible.", "Buen cuidado mejora premio.", "Recoge 1 objeto al volver.", "Chip abre esta tarjeta." },
  },
  {
    { "Low food = slip-up.", "Play raises joy.", "Bath cleans dirt.", "Petting gives joy/bond.", "High weight slows you.", "Candy cheers but fattens." },
    { "Sleep restores energy.", "Needs decay slower asleep.", "Light toggles sleep.", "Short PWR screen off.", "Power Save light-sleeps.", "No erase keeps saves." },
    { "Ball: tap the ball.", "Catch: tap icons.", "Memo: repeat sequence.", "Clean: tap stains.", "Type: pick advantage.", "Records and training." },
    { "Quick: lower damage.", "Enemy dodges less.", "You take less damage.", "Heavy: more damage.", "More risk/counterplay.", "Not always best." },
    { "Dodge avoids damage.", "Success: Counter ready.", "Next attack hits harder.", "Rest heals only 2x.", "Rest also gives Guard.", "Types change damage." },
    { "Pokedex: side swipe.", "Raised and caught count.", "10/25/50/100/151/160/200/251/300/386: frames.", "Profile: choose frame.", "Known detail: species chirp.", "SND ALL: tap pet." },
    { "Daily gives small goals.", "Events appear rarely.", "Wild battles are optional.", "Catch after winning.", "Streaks/medals persist.", "Shake to play. Walk gives JOY." },
    { "Expedition: 15/30/60 min.", "Energy is spent at start.", "Pet stays available.", "Care and bond improve finds.", "Claim 1 item when back.", "Chip opens this card." },
  },
  {
    { "Faim basse = erreur.", "Jouer monte la joie.", "Bain nettoie.", "Caresse donne lien/joie.", "Poids haut ralentit.", "Bonbon rend gros." },
    { "Sommeil rend energie.", "Besoins baissent moins.", "Lumiere dort/reveille.", "PWR court eteint ecran.", "Eco utilise light sleep.", "Sans erase garde save." },
    { "Balle: touche la balle.", "Attrape: touche icones.", "Memo: repete sequence.", "Nettoie: touche taches.", "Type: choisis avantage.", "Records et entrainement." },
    { "Rapide: degats bas.", "Ennemi esquive moins.", "Tu subis moins.", "Fort: degats hauts.", "Risque plus grand.", "Pas toujours meilleur." },
    { "Esquive evite degats.", "Succes: Contre pret.", "Prochaine attaque plus.", "Repos soigne 2 fois.", "Repos donne Garde.", "Types changent degats." },
    { "Pokedex: glisse cote.", "Eleve et capture comptent.", "10/25/50/100/151/160/200/251/300/386: cadres.", "Profil: choisis cadre.", "Detail connu: chirp.", "SON TOUT: touche pet." },
    { "Quotidien donne buts.", "Events rares.", "Combats sauvages option.", "Capture apres victoire.", "Series/medailles restent.", "Secoue: joue. Marche = JOIE." },
    { "Expedition: 15/30/60 min.", "Energie payee au depart.", "Le pet reste disponible.", "Soin/lien aide le butin.", "Prends 1 objet au retour.", "Chip ouvre cette carte." },
  },
  {
    { "Food 0 = Patzer.", "Spielen hebt Freude.", "Bad reinigt Hygiene.", "Streicheln gibt Bond.", "Hohes Gewicht bremst.", "Candy freut, macht dick." },
    { "Schlaf gibt Energie.", "Needs sinken langsamer.", "Licht: schlafen/wach.", "PWR kurz: Screen aus.", "Sparen nutzt Light Sleep.", "Ohne Erase bleibt Save." },
    { "Ball: Ball antippen.", "Fangen: Icons treffen.", "Memo: Folge merken.", "Putzen: Flecken tippen.", "Typ: Vorteil waehlen.", "Gibt Rekorde/Training." },
    { "Schnell: weniger Schaden.", "Gegner weicht selten aus.", "Du kassierst weniger.", "Stark: mehr Schaden.", "Mehr Risiko/Gegendruck.", "Nicht immer beste Wahl." },
    { "Ausweichen meidet Schaden.", "Klappt es: Konter bereit.", "Naechster Angriff staerker.", "Ruhen heilt nur 2x.", "Ruhen gibt auch Schutz.", "Typen aendern Schaden." },
    { "Pokedex: seitlich wischen.", "Aufz./gefangen zaehlen.", "10/25/50/100/151/160/200/251/300/386: Rahmen.", "Profil: Rahmen waehlen.", "Bekanntes Detail: Chirp.", "TON VIEL: Pet tippen." },
    { "Taeglich gibt Ziele.", "Events sind selten.", "Wildkampf ist optional.", "Fangen nach Sieg.", "Serien/Medaillen bleiben.", "Schuetteln spielt. Gehen = JOY." },
    { "Expedition: 15/30/60 Min.", "Kostet beim Start Energie.", "Pet bleibt verfuegbar.", "Pflege/Bond verbessert Fund.", "Fund danach einsammeln.", "Chip oeffnet diese Karte." },
  },
  {
    { "Cibo 0 = errore.", "Gioca aumenta gioia.", "Bagno pulisce.", "Carezza da legame.", "Peso alto rallenta.", "Dolce rallegra, ingrassa." },
    { "Sonno da energia.", "Bisogni calano meno.", "Luce dorme/sveglia.", "PWR corto spegne schermo.", "Risparmio usa light sleep.", "Senza erase salva." },
    { "Palla: tocca palla.", "Prendi: tocca icone.", "Memo: ripeti sequenza.", "Pulisci: tocca macchie.", "Tipo: scegli vantaggio.", "Record e allenamento." },
    { "Rapido: meno danni.", "Nemico schiva meno.", "Subisci meno danni.", "Forte: piu danni.", "Piu rischio.", "Non sempre migliore." },
    { "Schiva evita danni.", "Successo: contro pronto.", "Prox attacco piu forte.", "Riposo cura solo 2x.", "Riposo da Guardia.", "Tipi cambiano danni." },
    { "Pokedex: scorri lato.", "Allevato e preso contano.", "10/25/50/100/151/160/200/251/300/386: cornici.", "Profilo: scegli cornice.", "Dettaglio noto: chirp.", "SON TUTTO: tocca pet." },
    { "Quotidiano da obiettivi.", "Eventi rari.", "Lotte selvatiche opz.", "Cattura dopo vittoria.", "Serie/medaglie restano.", "Scuoti: gioca. Cammina = GIO." },
    { "Spedizione: 15/30/60 min.", "Energia spesa alla partenza.", "Il pet resta disponibile.", "Cura/legame migliora premio.", "Ritira 1 oggetto al ritorno.", "Chip apre questa carta." },
  },
  {
    { "Comida 0 = falha.", "Jogar sobe alegria.", "Banho limpa.", "Carinho da vinculo.", "Peso alto atrasa.", "Doce alegra, engorda." },
    { "Sono da energia.", "Necessidades caem menos.", "Luz dorme/acorda.", "PWR curto apaga tela.", "Poupanca usa light sleep.", "Sem erase guarda save." },
    { "Bola: toque na bola.", "Pegar: toque icones.", "Memo: repita sequencia.", "Limpa: toque manchas.", "Tipo: escolha vantagem.", "Recordes e treino." },
    { "Rapido: dano menor.", "Rival desvia menos.", "Voce recebe menos.", "Forte: dano maior.", "Mais risco.", "Nem sempre melhor." },
    { "Desviar evita dano.", "Sucesso: contra pronto.", "Prox ataque mais forte.", "Descanso cura so 2x.", "Descanso da Guarda.", "Tipos mudam dano." },
    { "Pokedex: deslize lado.", "Criado e apanhado contam.", "10/25/50/100/151/160/200/251/300/386: molduras.", "Perfil: escolha moldura.", "Detalhe conhecido: chirp.", "SOM TODO: toque pet." },
    { "Diario da metas.", "Eventos sao raros.", "Batalha selvagem opc.", "Captura apos vitoria.", "Series/medalhas ficam.", "Agita: joga. Andar da ALE." },
    { "Expedicao: 15/30/60 min.", "Energia gasta ao sair.", "Pet fica disponivel.", "Cuidado/laco melhora premio.", "Recolhe 1 item ao voltar.", "Chip abre este cartao." },
  },
};

void HelpScreen::render() {
  helpDirty = false;
  gfx->fillScreen(UI_BG_DAY);
  uint8_t lang = (gLang < LANG_COUNT) ? (uint8_t)gLang : (uint8_t)LANG_EN;
  if (helpPage >= HELP_PAGE_COUNT) helpPage = 0;

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  const char *h = HELP_WORD[lang];
  gfx->setCursor(SCREEN_CX - strlen(h) * 9, 36);
  gfx->print(h);

  gfx->setTextSize(2);
  gfx->setTextColor(UI_BAR_BAD);
  const char *title = HELP_TITLES[lang][helpPage];
  gfx->setCursor(SCREEN_CX - strlen(title) * 6, 76);
  gfx->print(title);

  gfx->setTextColor(UI_INK);
  for (uint8_t i = 0; i < HELP_LINE_COUNT; i++) {
    const char *line = HELP_LINES[lang][helpPage][i];
    gfx->setCursor(SCREEN_CX - strlen(line) * 6, 116 + i * 34);
    gfx->print(line);
  }

  char pg[12];
  snprintf(pg, sizeof(pg), "%u/%u", helpPage + 1, HELP_PAGE_COUNT);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(SCREEN_CX - strlen(pg) * 6, 340);
  gfx->print(pg);

  if (helpPage > 0) {
    gfx->fillRoundRect(48, 398, 82, 42, 12, UI_WHITE);
    gfx->drawRoundRect(48, 398, 82, 42, 12, UI_INK);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(72, 412);
    gfx->print("<<");
  }
  gfx->fillRoundRect(154, 398, 158, 42, 12, UI_BAR_OK);
  gfx->setTextColor(UI_BG_DAY);
  gfx->setTextSize(2);
  const char *ok = HELP_OK[lang];
  gfx->setCursor(154 + (158 - (int)strlen(ok) * 12) / 2, 412);
  gfx->print(ok);
  if (helpPage + 1 < HELP_PAGE_COUNT) {
    gfx->fillRoundRect(336, 398, 82, 42, 12, UI_WHITE);
    gfx->drawRoundRect(336, 398, 82, 42, 12, UI_INK);
    gfx->setTextColor(UI_INK);
    gfx->setCursor(360, 412);
    gfx->print(">>");
  }
  flushStampedFrame();
}

void HelpScreen::handleTap(int16_t x, int16_t y) {
  if (y >= 392 && y <= 448) {
    if (x >= 48 && x <= 130 && helpPage > 0) {
      helpPage--;
      helpDirty = true;
      sfxPlay(SFX_MENU);
      return;
    }
    if (x >= 336 && x <= 418 && helpPage + 1 < HELP_PAGE_COUNT) {
      helpPage++;
      helpDirty = true;
      sfxPlay(SFX_MENU);
      return;
    }
    if (x >= 154 && x <= 312) {
      helpOpen = false;
      clockOpen = true;
      clockDirty = true;
      setScreen(&clockSettingsScreen);
      sfxPlay(SFX_TAP);
      return;
    }
  }
  if (x > SCREEN_CX && helpPage + 1 < HELP_PAGE_COUNT) { helpPage++; helpDirty = true; sfxPlay(SFX_MENU); return; }
  if (x < SCREEN_CX && helpPage > 0) { helpPage--; helpDirty = true; sfxPlay(SFX_MENU); return; }
}

const char *helpLabelForLanguage(uint8_t lang) {
  return HELP_WORD[lang];
}

void HelpScreen::handleSwipe(int dir) {
  if (dir < 0 && helpPage + 1 < HELP_PAGE_COUNT) { helpPage++; helpDirty = true; }
  else if (dir > 0 && helpPage > 0) { helpPage--; helpDirty = true; }
  sfxPlay(SFX_MENU);
}

void HelpScreen::handleSwipeV(int) {
  helpOpen = false;
  clockOpen = true;
  clockDirty = true;
  setScreen(&clockSettingsScreen);
  sfxPlay(SFX_TAP);
}