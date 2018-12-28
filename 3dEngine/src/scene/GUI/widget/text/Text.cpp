#include <GL/glew.h>
#include <sstream>
#include <memory>

#include "scene/GUI/widget/text/Text.h"
#include "scene/GUI/GUISkinService.h"
#include "scene/GUI/widget/Size.h"
#include "resources/MediaManager.h"
#include "utils/display/quad/QuadDisplayerBuilder.h"

namespace urchin
{

	Text::Text(Position position, const std::string &fontFilename) :
		Widget(position, Size(0, 0, Size::PIXEL)),
		text(""),
		maxLength(-1)
	{
		font = MediaManager::instance()->getMedia<Font>(fontFilename);

		setText(text);
	}

	Text::~Text()
	{
		font->release();
	}

	void Text::createOrUpdateWidget()
	{
		//nothing to do: text cannot be defined in percentage (Size::SizeType::PERCENTAGE).
	}

	void Text::setText(const std::string &text, int maxLength)
	{
		this->text = text;
		this->maxLength = maxLength;

		//cut the text if needed
		unsigned int numLetters = 0;
		std::stringstream cuttedTextStream((maxLength>0) ? cutText(text, maxLength) : text);
		std::string item;
		std::vector<std::string> cutTextLines;
		while (std::getline(cuttedTextStream, item, '\n'))
		{
			cutTextLines.push_back(item);
			numLetters += item.size();
		}

		//creates the vertex array and texture array
		vertexData.resize(numLetters*8);
		textureData.resize(numLetters*8);
		unsigned int vertexIndex = 0, stIndex = 0;
		unsigned int width = 0;
		int offsetY = 0;
		for (auto &cutTextLine : cutTextLines)
		{ //each lines
			unsigned int offsetX = 0;
			for (char charLetter : cutTextLine)
			{ //each letters
				auto letter = static_cast<unsigned char>(charLetter);

				float t = (float)((letter)>>4) / 16.0f;
				float s = (float)((letter)%16) / 16.0f;

				vertexData[vertexIndex++] = offsetX;
				vertexData[vertexIndex++] = -font->getGlyph(letter).shift + offsetY;
				vertexData[vertexIndex++] = font->getGlyph(letter).width + offsetX;
				vertexData[vertexIndex++] = -font->getGlyph(letter).shift + offsetY;
				vertexData[vertexIndex++] = font->getGlyph(letter).width + offsetX;
				vertexData[vertexIndex++] = font->getGlyph(letter).height - font->getGlyph(letter).shift + offsetY;
				vertexData[vertexIndex++] = offsetX;
				vertexData[vertexIndex++] = font->getGlyph(letter).height - font->getGlyph(letter).shift + offsetY;

				textureData[stIndex++] = s;
				textureData[stIndex++] = t;
				textureData[stIndex++] = s+((float)font->getGlyph(letter).width/font->getDimensionTexture());
				textureData[stIndex++] = t;
				textureData[stIndex++] = s+((float)font->getGlyph(letter).width/font->getDimensionTexture());
				textureData[stIndex++] = t+((float)font->getGlyph(letter).height/font->getDimensionTexture());
				textureData[stIndex++] = s;
				textureData[stIndex++] = t+((float)font->getGlyph(letter).height/font->getDimensionTexture());

				offsetX += font->getGlyph(letter).width + font->getSpaceBetweenLetters();

				width = std::max(width, offsetX - font->getSpaceBetweenLetters());
			}
			offsetY += font->getSpaceBetweenLines();
		}

        if(cutTextLines.empty())
        { //add fake line to compute height
            cutTextLines.emplace_back("");
        }
		unsigned int numberOfInterLines = cutTextLines.size() - 1;
		setSize(Size(width, cutTextLines.size()*font->getHeight() + numberOfInterLines*font->getSpaceBetweenLines(), Size::SizeType::PIXEL));

		quadDisplayer = std::make_unique<QuadDisplayerBuilder>()
				->numberOfQuad(numLetters)
				->vertexData(GL_INT, &vertexData[0], false)
				->textureData(GL_FLOAT, &textureData[0], false)
				->build();
	}

	std::string Text::cutText(const std::string &constText, int maxLength)
	{
		std::string text(constText);

		int lineLength = 0;
		int indexLastSpace = 0;
		int lengthFromLastSpace = 0;

		for(int numLetter=0; text[numLetter]!=0; numLetter++)
		{ //each letters
			auto letter = static_cast<unsigned char>(text[numLetter]);

			if(letter=='\n')
			{
				lineLength = 0;
				lengthFromLastSpace = 0;
			}else	if(letter==' ')
			{
				indexLastSpace = numLetter;
				lengthFromLastSpace = 0;
			}

			int lengthLetter = font->getGlyph(letter).width + font->getSpaceBetweenLetters();
			if(lineLength+lengthLetter >= maxLength)
			{ //cut line
				text[indexLastSpace] = '\n';
				lineLength = lengthFromLastSpace;
			}else
			{
				lineLength += lengthLetter;
				lengthFromLastSpace += lengthLetter;
			}
		}

		return text;
	}

	const std::string &Text::getText() const
	{
		return text;
	}

	const Font *Text::getFont()
	{
		return font;
	}

	void Text::display(int translateDistanceLoc, float invFrameRate)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, font->getTextureID());

		quadDisplayer->display();
		
		glDisable(GL_BLEND);

		Widget::display(translateDistanceLoc, invFrameRate);
	}

}
