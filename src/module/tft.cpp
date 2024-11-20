#include "tft.h"

TFT_eSPI *tft;
TFT_eSPI get_tft() {
	if (tft == nullptr)
		tft = new TFT_eSPI();

	return *tft;
}
