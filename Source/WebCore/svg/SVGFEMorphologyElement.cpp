/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2018-2022 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SVGFEMorphologyElement.h"

#include "FEMorphology.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(SVGFEMorphologyElement);

inline SVGFEMorphologyElement::SVGFEMorphologyElement(const QualifiedName& tagName, Document& document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
    ASSERT(hasTagName(SVGNames::feMorphologyTag));
    
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        PropertyRegistry::registerProperty<SVGNames::inAttr, &SVGFEMorphologyElement::m_in1>();
        PropertyRegistry::registerProperty<SVGNames::operatorAttr, MorphologyOperatorType, &SVGFEMorphologyElement::m_svgOperator>();
        PropertyRegistry::registerProperty<SVGNames::radiusAttr, &SVGFEMorphologyElement::m_radiusX, &SVGFEMorphologyElement::m_radiusY>();
    });
}

Ref<SVGFEMorphologyElement> SVGFEMorphologyElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGFEMorphologyElement(tagName, document));
}

void SVGFEMorphologyElement::setRadius(float x, float y)
{
    m_radiusX->setBaseValInternal(x);
    m_radiusY->setBaseValInternal(y);
    setSVGResourcesInAncestorChainAreDirty();
}

void SVGFEMorphologyElement::parseAttribute(const QualifiedName& name, const AtomString& value)
{
    if (name == SVGNames::operatorAttr) {
        MorphologyOperatorType propertyValue = SVGPropertyTraits<MorphologyOperatorType>::fromString(value);
        if (propertyValue != MorphologyOperatorType::Unknown)
            m_svgOperator->setBaseValInternal<MorphologyOperatorType>(propertyValue);
        return;
    }

    if (name == SVGNames::inAttr) {
        m_in1->setBaseValInternal(value);
        return;
    }

    if (name == SVGNames::radiusAttr) {
        if (auto result = parseNumberOptionalNumber(value)) {
            m_radiusX->setBaseValInternal(result->first);
            m_radiusY->setBaseValInternal(result->second);
        }
        return;
    }

    SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
}

bool SVGFEMorphologyElement::setFilterEffectAttribute(FilterEffect* effect, const QualifiedName& attrName)
{
    FEMorphology* morphology = static_cast<FEMorphology*>(effect);
    if (attrName == SVGNames::operatorAttr)
        return morphology->setMorphologyOperator(svgOperator());
    if (attrName == SVGNames::radiusAttr) {
        // Both setRadius functions should be evaluated separately.
        bool isRadiusXChanged = morphology->setRadiusX(radiusX());
        bool isRadiusYChanged = morphology->setRadiusY(radiusY());
        return isRadiusXChanged || isRadiusYChanged;
    }

    ASSERT_NOT_REACHED();
    return false;
}

void SVGFEMorphologyElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (PropertyRegistry::isKnownAttribute(attrName)) {
        InstanceInvalidationGuard guard(*this);
        if (attrName == SVGNames::inAttr)
            setSVGResourcesInAncestorChainAreDirty();
        else {
            ASSERT(attrName == SVGNames::operatorAttr || attrName == SVGNames::radiusAttr);
            primitiveAttributeChanged(attrName);
        }
        return;
    }

    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
}

RefPtr<FilterEffect> SVGFEMorphologyElement::filterEffect(const SVGFilter&, const FilterEffectVector&, const GraphicsContext&) const
{
    if (radiusX() < 0 || radiusY() < 0)
        return nullptr;

    return FEMorphology::create(svgOperator(), radiusX(), radiusY());
}

} // namespace WebCore
