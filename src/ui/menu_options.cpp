
#include "menu_options.h"
#include "radio.h"
#include "types.h"

namespace Menu {
menu_option_st<MODULATION_MODE> modulation_options[] = {{radio::modulation_names[AM], AM},           {radio::modulation_names[FM], FM},
                                                        {radio::modulation_names[WFM], WFM},         {radio::modulation_names[CW], CW},
                                                        {radio::modulation_names[SSB_LSB], SSB_LSB}, {radio::modulation_names[SSB_USB], SSB_USB}};

menu_option_st<radio::BAND> band_options[] = {{radio::bandNames[radio::BAND_AUTO], radio::BAND_AUTO}, {radio::bandNames[radio::BAND_70cm], radio::BAND_70cm},
                                              {radio::bandNames[radio::BAND_1m], radio::BAND_1m},     {radio::bandNames[radio::BAND_2m], radio::BAND_2m},
                                              {radio::bandNames[radio::AIRBAND], radio::AIRBAND},     {radio::bandNames[radio::BAND_FM], radio::BAND_FM},
                                              {radio::bandNames[radio::BAND_6m], radio::BAND_6m},     {radio::bandNames[radio::BAND_10m], radio::BAND_10m},
                                              {radio::bandNames[radio::BAND_11m], radio::BAND_11m},   {radio::bandNames[radio::BAND_12m], radio::BAND_12m},
                                              {radio::bandNames[radio::BAND_15m], radio::BAND_15m},   {radio::bandNames[radio::BAND_17m], radio::BAND_17m},
                                              {radio::bandNames[radio::BAND_20m], radio::BAND_20m},   {radio::bandNames[radio::BAND_30m], radio::BAND_30m},
                                              {radio::bandNames[radio::BAND_40m], radio::BAND_40m},   {radio::bandNames[radio::BAND_60m], radio::BAND_60m},
                                              {radio::bandNames[radio::BAND_80m], radio::BAND_80m},   {radio::bandNames[radio::BAND_160m], radio::BAND_160m},
                                              {radio::bandNames[radio::BAND_ALL], radio::BAND_ALL}};

menu_option_st<radio::IF_FILTER> if_filter_options[] = {
    {radio::IFFilterNames[radio::IF_FILTER_AUTO], radio::IF_FILTER_AUTO},     {radio::IFFilterNames[radio::IF_FILTER_300HZ], radio::IF_FILTER_300HZ},
    {radio::IFFilterNames[radio::IF_FILTER_3KHZ], radio::IF_FILTER_3KHZ},     {radio::IFFilterNames[radio::IF_FILTER_6KHZ], radio::IF_FILTER_6KHZ},
    {radio::IFFilterNames[radio::IF_FILTER_7_5KHZ], radio::IF_FILTER_7_5KHZ}, {radio::IFFilterNames[radio::IF_FILTER_9KHZ], radio::IF_FILTER_9KHZ},
    {radio::IFFilterNames[radio::IF_FILTER_15KHZ], radio::IF_FILTER_15KHZ},   {radio::IFFilterNames[radio::IF_FILTER_150KHZ], radio::IF_FILTER_150KHZ},
    {radio::IFFilterNames[radio::IF_FILTER_180KHZ], radio::IF_FILTER_180KHZ}};
const char *colorNames[] = {"Black",        "Grey darker", "Grey dark", "Grey ligh", "White",  "Navy",     "Green dark", "Cyan dark",
                            "Maroon",       "Olive",       "Blue",      "Green",     "Red",    "Magenta",  "Yellow",     "Orange",
                            "Green-yellow", "Pink",        "Brown",     "Gold",      "Silver", "Sky blue", "Violet"};

menu_option_st<uint16_t> color_options[] = {
    {"   ", C565_BLACK, C565_BLACK, C565_BLACK},
    {"   ", C565_GREY_DARKER, C565_GREY_DARKER, C565_GREY_DARKER},
    {"   ", C565_GREY_DARK, C565_GREY_DARK, C565_GREY_DARK},
    {"   ", C565_GREY_LIGHT, C565_GREY_LIGHT, C565_GREY_LIGHT},
    {"   ", C565_WHITE, C565_WHITE, C565_WHITE},
    {"   ", C565_NAVY, C565_NAVY, C565_NAVY},
    {"   ", C565_GREEN_DARK, C565_GREEN_DARK, C565_GREEN_DARK},
    {"   ", C565_CYAN_DARK, C565_CYAN_DARK, C565_CYAN_DARK},
    {"   ", C565_MAROON, C565_MAROON, C565_MAROON},
    {"   ", C565_OLIVE, C565_OLIVE, C565_OLIVE},
    {"   ", C565_BLUE, C565_BLUE, C565_BLUE},
    {"   ", C565_GREEN, C565_GREEN, C565_GREEN},
    {"   ", C565_RED, C565_RED, C565_RED},
    {"   ", C565_MAGENTA, C565_MAGENTA, C565_MAGENTA},
    {"   ", C565_YELLOW, C565_YELLOW, C565_YELLOW},
    {"   ", C565_ORANGE, C565_ORANGE, C565_ORANGE},
    {"   ", C565_GREENYELLOW, C565_GREENYELLOW, C565_GREENYELLOW},
    {"   ", C565_PINK, C565_PINK, C565_PINK},
    {"   ", C565_BROWN, C565_BROWN, C565_BROWN},
    {"   ", C565_GOLD, C565_GOLD, C565_GOLD},
    {"   ", C565_SILVER, C565_SILVER, C565_SILVER},
    {"   ", C565_SKYBLUE, C565_SKYBLUE, C565_SKYBLUE},
    {"   ", C565_VIOLET, C565_VIOLET, C565_VIOLET},
};

Signal navigation_signal;

} // namespace Menu
