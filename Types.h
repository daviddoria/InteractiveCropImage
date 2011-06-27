#ifndef TYPES_H
#define TYPES_H

#include "itkImage.h"
#include "itkVectorImage.h"

typedef itk::VectorImage<float,2> FloatVectorImageType;
typedef itk::VectorImage<unsigned char,2> UnsignedCharVectorImageType;

typedef itk::Image<float,2> FloatScalarImageType;
typedef itk::Image<unsigned char,2> UnsignedCharScalarImageType;

#endif