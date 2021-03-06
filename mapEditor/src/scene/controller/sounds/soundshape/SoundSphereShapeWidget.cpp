#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>

#include "SoundSphereShapeWidget.h"
#include "support/SpinBoxStyleHelper.h"

namespace urchin
{

	SoundSphereShapeWidget::SoundSphereShapeWidget(const SceneSound *sceneSound) :
			SoundShapeWidget(sceneSound)
	{
		QLabel *positionLabel = new QLabel("Position:");
		mainLayout->addWidget(positionLabel, 1, 0);

		QHBoxLayout *positionLayout = new QHBoxLayout();
		mainLayout->addLayout(positionLayout, 1, 1);
		positionX = new QDoubleSpinBox();
		positionLayout->addWidget(positionX);
		SpinBoxStyleHelper::applyDefaultStyleOn(positionX);
		connect(positionX, SIGNAL(valueChanged(double)), this, SLOT(updateSoundShape()));
		positionY = new QDoubleSpinBox();
		positionLayout->addWidget(positionY);
		SpinBoxStyleHelper::applyDefaultStyleOn(positionY);
		connect(positionY, SIGNAL(valueChanged(double)), this, SLOT(updateSoundShape()));
		positionZ = new QDoubleSpinBox();
		positionLayout->addWidget(positionZ);
		SpinBoxStyleHelper::applyDefaultStyleOn(positionZ);
		connect(positionZ, SIGNAL(valueChanged(double)), this, SLOT(updateSoundShape()));

		QLabel *radiusLabel = new QLabel("Radius:");
		mainLayout->addWidget(radiusLabel, 2, 0);

		radius = new QDoubleSpinBox();
		mainLayout->addWidget(radius, 2, 1);
		SpinBoxStyleHelper::applyDefaultStyleOn(radius);
		radius->setMinimum(0.0);
		connect(radius, SIGNAL(valueChanged(double)), this, SLOT(updateSoundShape()));
	}

	SoundSphereShapeWidget::~SoundSphereShapeWidget()
	{

	}

	std::string SoundSphereShapeWidget::getSoundShapeName() const
	{
		return SPHERE_SHAPE_LABEL;
	}

	void SoundSphereShapeWidget::doSetupShapePropertiesFrom(const SoundShape *shape)
	{
		const SoundSphere *sphereShape = static_cast<const SoundSphere *>(shape);

		positionX->setValue(sphereShape->getPosition().X);
		positionY->setValue(sphereShape->getPosition().Y);
		positionZ->setValue(sphereShape->getPosition().Z);

		radius->setValue(sphereShape->getRadius());
	}

	const SoundShape *SoundSphereShapeWidget::createSoundShape() const
	{
		Point3<float> position(positionX->value(), positionY->value(), positionZ->value());

		return new SoundSphere(radius->value(), position, getMarginValue());
	}
}
