/*=========================================================================
 *
 *  Copyright David Doria 2011 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "ui_Form.h"
#include "Form.h"

// ITK
#include "itkCastImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRegionOfInterestImageFilter.h"

// Qt
#include <QFileDialog>
#include <QIcon>

// VTK
#include <vtkSmartPointer.h>
#include <vtkPointData.h>
#include <vtkCommand.h>
#include <vtkMath.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkProperty2D.h>
#include <vtkBorderWidget.h>
#include <vtkBorderRepresentation.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkProperty.h>

// Custom
#include "Helpers.h"
#include "Types.h"

void Form::BorderCallback(vtkObject *caller, unsigned long vtkNotUsed(eventId),
                                void* vtkNotUsed(callData))
{
  vtkBorderWidget *borderWidget =
      reinterpret_cast<vtkBorderWidget*>(caller);

  // Get the world coordinates of the two corners of the box
  vtkCoordinate* lowerLeftCoordinate = static_cast<vtkBorderRepresentation*>(
      borderWidget->GetRepresentation())->GetPositionCoordinate();
  double* lowerLeft = lowerLeftCoordinate->GetComputedWorldValue(this->LeftRenderer);
  //std::cout << "Lower left coordinate: " << lowerLeft[0] << ", " << lowerLeft[1] << ", " << lowerLeft[2] << std::endl;

  vtkCoordinate* upperRightCoordinate = static_cast<vtkBorderRepresentation*>(
      borderWidget->GetRepresentation())->GetPosition2Coordinate();
  double* upperRight = upperRightCoordinate ->GetComputedWorldValue(this->LeftRenderer);
  //std::cout << "Upper right coordinate: " << upperRight[0] << ", " << upperRight[1] << ", " << upperRight[2] << std::endl;

  double* bounds = this->LeftImageActor->GetBounds();
  double imageXMin = bounds[0];
  double imageXMax = bounds[1];
  double imageYMin = bounds[2];
  double imageYMax = bounds[3];

  // First, assume the border widget is entirely inside the image
  double selectionXMin = lowerLeft[0];
  double selectionXMax = upperRight[0];
  double selectionYMin = lowerLeft[1];
  double selectionYMax = upperRight[1];

  // If the entire border widget is outside the image, return.
  if( (selectionXMax < imageXMin) || (selectionXMin > imageXMax) ||
      (selectionYMax < imageYMin) || (selectionYMin > imageYMax ) )
    {
    return;
    }

  // Don't allow the selection to be outside the valid range
  selectionXMin = std::max(selectionXMin,imageXMin);
  selectionXMax = std::min(selectionXMax,imageXMax);
  selectionYMin = std::max(selectionYMin,imageYMin);
  selectionYMax = std::min(selectionYMax,imageYMax);

  // Setup the region to be extracted
  itk::Index<2> extractionStart;
  extractionStart[0] = static_cast<unsigned int>(selectionXMin);
  extractionStart[1] = static_cast<unsigned int>(selectionYMin);

  itk::Size<2> extractionSize;
  extractionSize[0] = static_cast<unsigned int>(selectionXMax - selectionXMin);
  extractionSize[1] = static_cast<unsigned int>(selectionYMax - selectionYMin);

  itk::ImageRegion<2> regionToExtract(extractionStart, extractionSize);

  typedef itk::RegionOfInterestImageFilter< FloatVectorImageType, FloatVectorImageType > RegionOfInterestFilterType;
  RegionOfInterestFilterType::Pointer regionOfInterestFilter = RegionOfInterestFilterType::New();
  regionOfInterestFilter->SetRegionOfInterest(regionToExtract);
  regionOfInterestFilter->SetInput(this->Image);
  regionOfInterestFilter->Update();

  this->CroppedImage = regionOfInterestFilter->GetOutput();
  
  Helpers::ITKImagetoVTKImage(this->CroppedImage, this->RightImageData);
  
  this->RightImageActor->SetInput(this->RightImageData);

  // Add Actor to renderer
  this->RightRenderer->AddActor(this->RightImageActor);
  this->RightRenderer->ResetCamera();
  
  this->RightRenderer->Render();
  this->qvtkWidgetRight->GetRenderWindow()->Render();
}

// Constructor
Form::Form()
{
  this->setupUi(this);

  this->LeftRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->RightRenderer = vtkSmartPointer<vtkRenderer>::New();

  this->qvtkWidgetLeft->GetRenderWindow()->AddRenderer(this->LeftRenderer);
  this->qvtkWidgetRight->GetRenderWindow()->AddRenderer(this->RightRenderer);

  this->LeftImageActor = vtkSmartPointer<vtkImageActor>::New();
  this->LeftImageData = vtkSmartPointer<vtkImageData>::New();
  
  this->RightImageActor = vtkSmartPointer<vtkImageActor>::New();
  this->RightImageData = vtkSmartPointer<vtkImageData>::New();
  
  // Setup toolbar
  QIcon openIcon = QIcon::fromTheme("document-open");
  actionOpen->setIcon(openIcon);
  this->toolBar->addAction(actionOpen);
  
  QIcon saveIcon = QIcon::fromTheme("document-save");
  actionSave->setIcon(saveIcon);
  this->toolBar->addAction(actionSave);

  this->BorderWidget = vtkSmartPointer<vtkBorderWidget>::New();

};


void Form::on_actionOpen_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".", "Image Files (*.png *.mhd *.tif)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  typedef itk::ImageFileReader<FloatVectorImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(fileName.toStdString());
  reader->Update();
  
  this->Image = reader->GetOutput();

  if(this->chkRGB->isChecked())
    {
    Helpers::ITKImagetoVTKRGBImage(this->Image, this->LeftImageData);
    }
  else
    {
    Helpers::ITKImagetoVTKMagnitudeImage(this->Image, this->LeftImageData);
    }
  
  this->LeftImageActor->SetInput(this->LeftImageData);

  // Add Actor to renderer
  this->LeftRenderer->AddActor(this->LeftImageActor);
  this->LeftRenderer->ResetCamera();

  vtkSmartPointer<vtkInteractorStyleImage> interactorStyle =
      vtkSmartPointer<vtkInteractorStyleImage>::New();
  this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor()->SetInteractorStyle(interactorStyle);

  this->BorderWidget->SetInteractor(this->qvtkWidgetLeft->GetRenderWindow()->GetInteractor());
  static_cast<vtkBorderRepresentation*>(this->BorderWidget->GetRepresentation())->GetBorderProperty()->SetColor(1,0,0); //red

  //this->BorderWidget->AddObserver(vtkCommand::InteractionEvent,this->BorderCallback);
  //this->BorderWidget->AddObserver(vtkCommand::EndInteractionEvent,this->BorderCallback);
  this->BorderWidget->AddObserver(vtkCommand::EndInteractionEvent, this, &Form::BorderCallback);

  
  vtkCoordinate* lowerLeftCoordinate = static_cast<vtkBorderRepresentation*>(
      this->BorderWidget->GetRepresentation())->GetPositionCoordinate();
  lowerLeftCoordinate->SetCoordinateSystemToWorld();
  lowerLeftCoordinate->SetValue(0,0,0);
  
/*
  vtkCoordinate* upperRightCoordinate = static_cast<vtkBorderRepresentation*>(
      this->BorderWidget->GetRepresentation())->GetPosition2Coordinate();
  upperRightCoordinate->SetCoordinateSystemToWorld();
  upperRightCoordinate->SetValue(this->Image->GetLargestPossibleRegion().GetSize()[0],this->Image->GetLargestPossibleRegion().GetSize()[1],0);
  */
/*
 // This doesn't work
  static_cast<vtkBorderRepresentation*>(
      this->BorderWidget->GetRepresentation())->SetPosition(0,0);
  static_cast<vtkBorderRepresentation*>(
      //this->BorderWidget->GetRepresentation())->SetPosition2(upperRightCoordinate);
    this->BorderWidget->GetRepresentation())->SetPosition2(this->Image->GetLargestPossibleRegion().GetSize()[0],this->Image->GetLargestPossibleRegion().GetSize()[1]);
  std::cout << "(" << this->Image->GetLargestPossibleRegion().GetSize()[0] << " , " << this->Image->GetLargestPossibleRegion().GetSize()[1] << std::endl;
*/

  this->BorderWidget->On();
  this->BorderWidget->ResizableOn();

  // Call the interaction event so the cropped region (right qvtkwidget) is updated
  BorderCallback(this->BorderWidget, 0, NULL);
  this->RightRenderer->ResetCamera();

}

void Form::on_actionSave_activated()
{
  if(this->chkRGB->isChecked())
    {
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".", "Image Files (*.png)");
    std::cout << "Got filename: " << fileName.toStdString() << std::endl;
    if(fileName.toStdString().empty())
      {
      std::cout << "Filename was empty." << std::endl;
      return;
      }
    typedef itk::CastImageFilter< FloatVectorImageType, UnsignedCharVectorImageType > CastFilterType;
    CastFilterType::Pointer castFilter = CastFilterType::New();
    castFilter->SetInput(this->CroppedImage);
    castFilter->Update();
    
    typedef  itk::ImageFileWriter< UnsignedCharVectorImageType  > WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(fileName.toStdString());
    writer->SetInput(castFilter->GetOutput());
    writer->Update();
    }
  else
    {
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".", "Image Files (*.mhd)");
    std::cout << "Got filename: " << fileName.toStdString() << std::endl;
    if(fileName.toStdString().empty())
      {
      std::cout << "Filename was empty." << std::endl;
      return;
      }
    typedef  itk::ImageFileWriter< FloatVectorImageType  > WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(fileName.toStdString());
    writer->SetInput(this->CroppedImage);
    writer->Update();
    }

}
