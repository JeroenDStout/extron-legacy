#include "extron_core/data.h"


using namespace extron::core;


extron_data::extron_data()
{
    this->clear();
    
    day_offset_and_weight.push_back({ -2, .08f });
    day_offset_and_weight.push_back({ -1, .15f });
    day_offset_and_weight.push_back({  0, .30f });
    day_offset_and_weight.push_back({  1, .20f });
    day_offset_and_weight.push_back({  2, .15f });
    day_offset_and_weight.push_back({  3, .08f });
    day_offset_and_weight.push_back({  4, .04f });
}


extron_data::~extron_data()
{
}


void extron_data::clear()
{
    // TBA
}