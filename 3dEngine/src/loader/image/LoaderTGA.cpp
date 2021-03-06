#include <stdexcept>
#include <fstream>
#include "UrchinCommon.h"

#include "loader/image/LoaderTGA.h"

namespace urchin
{
	
	LoaderTGA::LoaderTGA() : Loader<Image>(),
			colorMap(nullptr),
			data(nullptr),
			width(0),
			height(0),
			componentsCount(0),
			format(Image::IMAGE_RGB)
	{

	}

	Image *LoaderTGA::loadFromFile(const std::string &filename)
	{
		//opens file
		std::ifstream file;
		std::string filenamePath = FileSystem::instance()->getResourcesDirectory() + filename;
		file.open(filenamePath, std::ios::in | std::ios::binary);
		if(file.fail())
		{
			throw std::invalid_argument("Cannot open the file " + filenamePath + ".");
		}

		//gets file length
		file.seekg(0, std::ios::end);
		int length = file.tellg();
		file.seekg(0, std::ios::beg);
		
		//extracts header
		TgaHeader header{};
		file.read((char *)&header, sizeof(TgaHeader));
		file.seekg(header.idLenght, std::ios::cur);
			
		//extracts color map (color map is stored in BGR format)
		if(header.colormapType)
		{
			colorMap = new unsigned char[header.cmLength*(header.cmSize>>3)];
			file.read((char *)colorMap, header.cmLength*(header.cmSize>>3));
		}

		//memory allocation for rough pixel data
		int lengthData = length - file.tellg();
		data = new unsigned char[lengthData];
		file.read((char *)data, lengthData);

		//memory allocation for pixel data
		getImageInfo(header);
		texels.resize(width*height*componentsCount, 0);

		//reads image data
		switch(header.imageType)
		{
			case 1:
				//uncompressed 8 bits color index
				readTGA8bits();
				break;

			case 2:
				//uncompressed 16-24-32 bits
				switch(header.pixelDepth)
				{
					case 16:
						readTGA16bits();
						break;

					case 24:
						readTGA24bits();
						break;

					case 32:
						readTGA32bits();
						break;
					default:
						break;
				}
				break;

			case 3:
				//uncompressed 8 bits grayscale
				if(header.pixelDepth==8)
				{
					readTGAgray8bits();
				}else
				{
					throw std::runtime_error("Wrong number of bits for grayscale: " + std::to_string(header.pixelDepth));
				}
				break;

			case 9:
				//RLE compressed 8 bits color index
				readTGA8bitsRLE();
				break;

			case 10:
				//RLE compressed 16-24-32 bits
				switch(header.pixelDepth)
				{
					case 16:
						readTGA16bitsRLE();
						break;

					case 24:
						readTGA24bitsRLE();
						break;

					case 32:
						readTGA32bitsRLE();
						break;
					default:
						break;
				}
				break;

			case 11:
				//RLE compressed 8bits grayscale
				if(header.pixelDepth == 8)
				{
					readTGAgray8bitsRLE();
				}else
				{
					throw std::runtime_error("Wrong number of bits for grayscale: " + std::to_string(header.pixelDepth));
				}
				break;

			default:
				if(header.colormapType)
				{
					delete [] colorMap;
				}
				delete [] data;
				//image type is not correct
				throw std::runtime_error("Unknown TGA image type, filename: " + filenamePath + ".");
		}
		
		if(header.colormapType)
		{
			delete [] colorMap;
		}
		delete [] data;
		
		
		short origin = ((int)header.imageDescriptor & 0x20)>>5; //0:origin bottom, 1:origin top
		if(origin==0)
		{ //inverses the texels
			std::vector<unsigned char> texelsInverse(width*height*componentsCount, 0);
			
			for(unsigned int i=0, iInverse=height-1;i<height;i++, iInverse--)
			{
				for(unsigned int j=0;j<width*componentsCount;j++)
				{
					texelsInverse[i*(width*componentsCount) + j] = texels[iInverse*(width*componentsCount) + j];
				}
			}

			return new Image(width, height, format, std::move(texelsInverse));
		}

		return new Image(width, height, format, std::move(texels));
	}

	void LoaderTGA::getImageInfo(const TgaHeader &header)
	{
		width = static_cast<unsigned int>(header.width);
		height = static_cast<unsigned int>(header.height);

		switch(header.imageType)
		{
			case 3:  //grayscale 8 bits
			case 11: //grayscale 8 bits (RLE)
			{
				if(header.pixelDepth==8)
				{
					format = Image::IMAGE_GRAYSCALE;
					componentsCount = 1;
				}else
				{
					throw std::runtime_error("Wrong number of bits for grayscale: " + std::to_string(header.pixelDepth));
				}

				break;
			}

			case 1:  //8 bits color index
			case 2:  //BGR 16-24-32 bits
			case 9:  //8 bits color index (RLE)
			case 10: //BGR 16-24-32 bits (RLE)
			{
				//8 bits and 16 bits images will be converted to 24 bits
				if(header.pixelDepth<=24)
				{
					format = Image::IMAGE_RGB;
					componentsCount = 3;
				}else //32 bits
				{
					format = Image::IMAGE_RGBA;
					componentsCount = 4;
				}

				break;
			}
			default:
				break;
		}
	}

	void LoaderTGA::readTGA8bits()
	{
		unsigned char color;

		for(unsigned int i=0; i < width*height;++i)
		{
			//reads index color byte
			color = data[i];

			//converts to RGB 24 bits
			texels[(i*3)+2] = colorMap[(color*3)+0];
			texels[(i*3)+1] = colorMap[(color*3)+1];
			texels[(i*3)+0] = colorMap[(color*3)+2];
		}
	}

	void LoaderTGA::readTGA16bits()
	{
		unsigned short color;

		for(unsigned int i=0, j=0;i<width*height;++i, j+=2)
		{
			//reads color word
			color = data[j] + (data[j+1] << 8);

			//converts BGR to RGB
			texels[(i*3)+0] = (unsigned char)(((color & 0x7C00) >> 10) << 3);
			texels[(i*3)+1] = (unsigned char)(((color & 0x03E0) >>  5) << 3);
			texels[(i*3)+2] = (unsigned char)(((color & 0x001F) >>  0) << 3);
		}
	}

	void LoaderTGA::readTGA24bits()
	{
		for(unsigned int i=0, j=0; i<width*height; ++i, j+=3)
		{
			//reads and converts BGR to RGB
			texels[(i*3)+2] = data[j+0];
			texels[(i*3)+1] = data[j+1];
			texels[(i*3)+0] = data[j+2];
		}
	}

	void LoaderTGA::readTGA32bits()
	{
		for(unsigned int i=0,j=0;i<width*height;++i, j+=4)
		{
			//reads and converts BGRA to RGBA
			texels[(i*4)+2] = data[j+0];
			texels[(i*4)+1] = data[j+1];
			texels[(i*4)+0] = data[j+2];
			texels[(i*4)+3] = data[j+3];
		}
	}

	void LoaderTGA::readTGAgray8bits()
	{
		for(unsigned int i=0; i<width*height; ++i)
		{
			texels[i] = data[i];
		}
	}

	void LoaderTGA::readTGA8bitsRLE()
	{
		int j = 0;
		unsigned char color;
		unsigned char packetHeader;
		unsigned int ptrIndex = 0;

		while(ptrIndex < width*height*3)
		{
			//reads first byte
			packetHeader = data[j++];
			int size = 1 + (packetHeader & 0x7f);

			if(packetHeader & 0x80)
			{
				//run-length packet
				color = data[j++];

				for(int i=0;i<size;++i, ptrIndex+=3)
				{
					texels[ptrIndex] = colorMap[(color * 3) + 2];
					texels[ptrIndex+1] = colorMap[(color * 3) + 1];
					texels[ptrIndex+2] = colorMap[(color * 3) + 0];
				}
			}else
			{
				//non run-length packet
				for(int i=0;i<size;++i,ptrIndex+=3)
				{
					color = data[j++];

					texels[ptrIndex] = colorMap[(color * 3) + 2];
					texels[ptrIndex+1] = colorMap[(color * 3) + 1];
					texels[ptrIndex+2] = colorMap[(color * 3) + 0];
				}
			}
		}
	}

	void LoaderTGA::readTGA16bitsRLE()
	{
		int j = 0;
		unsigned short color;
		unsigned char packetHeader;
		unsigned int ptrIndex = 0;

		while(ptrIndex < width*height*3)
		{
			//reads first byte
			packetHeader = data[j++];
			int size = 1 + (packetHeader & 0x7f);

			if(packetHeader & 0x80)
			{
				//run-length packet
				color = data[j] + (data[j + 1] << 8);
				j+=2;

				for(int i=0; i<size; ++i,ptrIndex+=3)
				{
					texels[ptrIndex] = (unsigned char)(((color & 0x7C00) >> 10) << 3);
					texels[ptrIndex+1] = (unsigned char)(((color & 0x03E0) >>  5) << 3);
					texels[ptrIndex+2] = (unsigned char)(((color & 0x001F) >>  0) << 3);
				}
			}else
			{
				//non run-length packet
				for(int i=0; i<size; ++i,ptrIndex+=3,j+=2)
				{
					color = data[j] + (data[j + 1] << 8);

					texels[ptrIndex] = (unsigned char)(((color & 0x7C00) >> 10) << 3);
					texels[ptrIndex+1] = (unsigned char)(((color & 0x03E0) >> 5) << 3);
					texels[ptrIndex+2] = (unsigned char)(((color & 0x001F) >> 0) << 3);
				}
			}
		}
	}

	void LoaderTGA::readTGA24bitsRLE()
	{
		int j = 0;
		unsigned char *rgb;
		unsigned char packetHeader;
		unsigned int ptrIndex = 0;

		while(ptrIndex < width*height*3)
		{
			//reads first byte
			packetHeader = data[j++];
			int size = 1 + (packetHeader & 0x7f);

			if(packetHeader & 0x80)
			{
				//run-length packet
				rgb = &data[j];
				j += 3;

				for(int i=0; i<size; ++i,ptrIndex+=3)
				{
					texels[ptrIndex] = rgb[2];
					texels[ptrIndex+1] = rgb[1];
					texels[ptrIndex+2] = rgb[0];
				}
			}else
			{
				//non run-length packet
				for(int i=0; i<size; ++i,ptrIndex+=3,j+=3)
				{
					texels[ptrIndex+2] = data[j + 0];
					texels[ptrIndex+1] = data[j + 1];
					texels[ptrIndex] = data[j + 2];
				}
			}
		}
	}

	void LoaderTGA::readTGA32bitsRLE()
	{
		int j = 0;
		unsigned char *rgba;
		unsigned char packetHeader;
		unsigned int ptrIndex = 0;

		while(ptrIndex < width*height*4)
		{
			//reads first byte
			packetHeader = data[j++];
			int size = 1 + (packetHeader & 0x7f);

			if(packetHeader & 0x80)
			{
				//run-length packet
				rgba = &data[j];
				j += 4;

				for(int i=0; i<size; ++i,ptrIndex+=4)
				{
					texels[ptrIndex] = rgba[2];
					texels[ptrIndex+1] = rgba[1];
					texels[ptrIndex+2] = rgba[0];
					texels[ptrIndex+3] = rgba[3];
				}
			}else
			{
				//non run-length packet
				for(int i=0; i<size; ++i,ptrIndex+=4,j+=4)
				{
					texels[ptrIndex+2] = data[j + 0];
					texels[ptrIndex+1] = data[j + 1];
					texels[ptrIndex] = data[j + 2];
					texels[ptrIndex+3] = data[j + 3];
				}
			}
		}
	}

	void LoaderTGA::readTGAgray8bitsRLE()
	{
		int j = 0;
		unsigned char color;
		unsigned char packetHeader;
		unsigned int ptrIndex = 0;

		while(ptrIndex < width*height)
		{
			//reads first byte
			packetHeader = data[j++];
			int size = 1+(packetHeader & 0x7f);

			if(packetHeader & 0x80)
			{
				//run-length packet
				color = data[j++];

				for(int i=0; i<size; ++i,ptrIndex++)
				{
					texels[ptrIndex]=color;
				}
			}else
			{
				//non run-length packet
				for(unsigned int i=0; i<size; ++i)
				{
					texels[ptrIndex+i] = data[j+i];
				}

				ptrIndex+=size;
				j+=size;
			}
		}
	}

}
