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

#ifndef FORM_H
#define FORM_H

#include "ui_Form.h"

// VTK
#include <vtkSmartPointer.h>

// ITK
#include "itkImage.h"

// Qt
#include <QMainWindow>

// Custom
#include "Types.h"

// Forward declarations
class vtkRenderer;
class vtkBorderWidget;
class BorderCallbackClass;
class vtkImageData;
class vtkImageActor;
class vtkActor;

class Form : public QMainWindow, public Ui::Form
{
  Q_OBJECT
public:

  // Constructor/Destructor
  Form();
  ~Form() {};

  void MainFunction();

  void SetupImage();
  void SetupLidar();

  void BorderCallback(vtkObject* caller,
                    long unsigned int eventId,
                    void* callData );

public slots:
  void on_actionOpen_activated();
  void on_actionSave_activated();

protected:

  vtkSmartPointer<vtkBorderWidget> BorderWidget;

  vtkSmartPointer<vtkRenderer> LeftRenderer;
  vtkSmartPointer<vtkImageActor> LeftImageActor;
  vtkSmartPointer<vtkImageData> LeftImageData;
  
  vtkSmartPointer<vtkRenderer> RightRenderer;
  vtkSmartPointer<vtkImageActor> RightImageActor;
  vtkSmartPointer<vtkImageData> RightImageData;
  
  FloatVectorImageType::Pointer Image;
  FloatVectorImageType::Pointer CroppedImage;
  
};

#endif // Form_H
