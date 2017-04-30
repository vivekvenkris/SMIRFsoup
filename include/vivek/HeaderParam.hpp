/*
 * HeaderParam.h
 *
 *  Created on: Aug 22, 2016
 *      Author: vivek
 */

#ifndef HEADERPARAM_H_
#define HEADERPARAM_H_

#include <string>
#include <iostream>

namespace vivek{
class HeaderParamBase;
template <class T> class HeaderParam;
}

class vivek::HeaderParamBase {
public:
	bool inheader;
	std::string dtype;
	std::string key;
	bool operator== (const HeaderParamBase& d) const;

	 virtual ~HeaderParamBase(){}
};

template <class T> class vivek::HeaderParam : public vivek::HeaderParamBase {

public:
	T value;

	inline HeaderParam(std::string key, std::string dtype) {
		this->key = key;
		this->dtype = dtype;
		this->inheader = false;

	}

	inline HeaderParam(std::string key,std::string dtype, T value) {
		this->key = key;
		this->dtype = dtype;
		this->value = value;
		this->inheader = true;

	}

	inline HeaderParam( HeaderParamBase* param_base){

		HeaderParam<T>* param = dynamic_cast<HeaderParam<T> * > (param_base);
		if(param ==NULL) {
			std::cerr<< "param is null. Possibly an incorrect dynamic cast??" << std::endl;
			return;
		}

		this->key = param->key;
		this->dtype = param->dtype;
		this->value = param->value;
		this->inheader = param->inheader;

	}

	inline ~HeaderParam(){}
};


#endif /* HEADERPARAM_H_ */
