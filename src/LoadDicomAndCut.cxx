/*=========================================================================
 *
 *  Copyright Insight Software Consortium
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
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"

//Filter for cutting
#include "itkMaskNegatedImageFilter.h"


#include <vector>
#include "itksys/SystemTools.hxx"
int main( int argc, char* argv[] )
{
  if( argc < 4 )
  {
    std::cerr << "Usage: " << argv[0] <<
      " DicomDirectory  OutputDicomDirectory maskFile" << std::endl;
    return EXIT_FAILURE;
  }
  typedef signed short    PixelType;
  const unsigned int      Dimension = 3;
  typedef itk::Image< PixelType, Dimension >      ImageType;
  typedef itk::ImageSeriesReader< ImageType >     ReaderType;

  typedef itk::GDCMImageIO                        ImageIOType;
  typedef itk::GDCMSeriesFileNames                NamesGeneratorType;
  ImageIOType::Pointer gdcmIO = ImageIOType::New();
  NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();

  namesGenerator->SetInputDirectory( argv[1] );
  const ReaderType::FileNamesContainer & filenames =
                            namesGenerator->GetInputFileNames();

  const unsigned int numberOfFileNames =  filenames.size();
  std::cout << numberOfFileNames << std::endl;
  for(unsigned int fni = 0; fni < numberOfFileNames; ++fni)
  {
    std::cout << "filename # " << fni << " = ";
    std::cout << filenames[fni] << std::endl;
  }
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetImageIO( gdcmIO );
  reader->SetFileNames( filenames );

  try
  {
    reader->Update();
  }
  catch (itk::ExceptionObject &excp)
  {
    std::cerr << "Exception thrown while writing the image" << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
  }

  ImageType::Pointer inputImage = reader->GetOutput();

  //get mask
  ReaderType::Pointer maskReader = ReaderType::New();
  maskReader->SetFileName(argv[3]);
  maskReader->Update();

  //remove mask from input image
  typedef itk::MaskNegatedImageFilter< ImageType, ImageType, ImageType > MaskFilterType;
  MaskFilterType::Pointer maskFilter = MaskFilterType::New();
  maskFilter->SetInput(inputImage);
  maskFilter->SetMaskImage(maskReader->GetOutput());
  maskFilter->SetOutsideValue(-1000);
  maskFilter->Update();

  //write to outputDirectory
  const char * outputDirectory = argv[2];
  itksys::SystemTools::MakeDirectory( outputDirectory );
  typedef signed short    OutputPixelType;
  const unsigned int      OutputDimension = 2;
  typedef itk::Image< OutputPixelType, OutputDimension >    Image2DType;
  typedef itk::ImageSeriesWriter<ImageType, Image2DType >  SeriesWriterType;
  SeriesWriterType::Pointer seriesWriter = SeriesWriterType::New();

  //write identical headers also with original UID's -> besides the changed pixel values the files look the same!
  seriesWriter->SetInput( maskFilter->GetOutput() );
  gdcmIO->SetKeepOriginalUID(true);
  seriesWriter->SetImageIO( gdcmIO );
  namesGenerator->SetOutputDirectory( outputDirectory );
  seriesWriter->SetFileNames( namesGenerator->GetOutputFileNames() );
  seriesWriter->SetMetaDataDictionaryArray(reader->GetMetaDataDictionaryArray() );
  try
  {
    seriesWriter->Update();
  }
  catch( itk::ExceptionObject & excp )
  {
    std::cerr << "Exception thrown while writing the series " << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}