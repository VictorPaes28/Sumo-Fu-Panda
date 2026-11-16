#ifndef TAPA_LATERAL_H
#define TAPA_LATERAL_H

#include "corpo.h"

/* Assistencia fisica do tapa a cada passo. Arco e peso variam de zero a um. */
void corpo_guiar_tapa(Corpo *c, const Corpo *oponente, float arco, float peso, bool direita);

#endif
