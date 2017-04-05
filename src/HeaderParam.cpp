#include "../include/vivek/HeaderParam.hpp"
bool vivek::HeaderParamBase::operator== (const HeaderParamBase& d) const{
	std::cerr << this->key << " " << d.key <<std::endl;
	return (this->key == d.key);
}
