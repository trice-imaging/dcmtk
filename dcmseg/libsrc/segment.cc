/*
 *
 *  Copyright (C) 2015, Open Connections GmbH
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation are maintained by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module:  dcmseg
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Class representing a Segment from the Segment Ident. Sequence
 *
 */
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmseg/segment.h"
#include "dcmtk/dcmseg/segtypes.h"
#include "dcmtk/dcmseg/segdoc.h"


OFCondition DcmSegment::create(DcmSegment*& segment,
                               const OFString& segmentLabel,
                               const CodeSequenceMacro& segmentedPropertyCategory,
                               const CodeSequenceMacro& segmentedPropertyType,
                               const DcmSegTypes::E_SegmentAlgoType algoType,
                               const OFString& algoName)

{
  segment = new DcmSegment();
  if (segment == NULL)
    return EC_MemoryExhausted;

  OFCondition result = segment->setSegmentLabel(segmentLabel, OFTrue /* check value */);

  if (result.good())
  {
    segment->m_SegmentDescription.getSegmentedPropertyCategoryCode() = segmentedPropertyCategory;
    result = segment->m_SegmentDescription.getSegmentedPropertyCategoryCode().check();
  }

  if (result.good())
  {
    segment->m_SegmentDescription.getSegmentedPropertyTypeCode() = segmentedPropertyType;
    result = segment->getSegmentedPropertyTypeCode().check();
  }


  if (result.good())
  {
      result = segment->setSegmentAlgorithm(algoType, algoName, OFTrue);
  }

  if ( result.bad() )
  {
    delete segment;
    segment = NULL;
  }

  return result;
}



OFCondition DcmSegment::read(DcmItem& item,
                             const OFBool clearOldData)
{
  if (clearOldData)
    clearData();

  m_SegmentDescription.read(item);
  DcmIODUtil::getAndCheckElementFromDataset(item, m_SegmentAlgorithmName, m_Rules.getByTag(DCM_SegmentAlgorithmName));

  DcmIODUtil::readSingleItem<AlgorithmIdentificationMacro>(
    item,
    DCM_SegmentSurfaceGenerationAlgorithmIdentificationSequence,
    m_SegmentSurfaceGenerationAlgorithmIdentification,
    "3",
    "Segmentation Image Module");

  DcmIODUtil::getAndCheckElementFromDataset(item, m_RecommendedDisplayGrayscaleValue, m_Rules.getByTag(DCM_RecommendedDisplayGrayscaleValue));
  DcmIODUtil::getAndCheckElementFromDataset(item, m_RecommendedDisplayCIELabValue, m_Rules.getByTag(DCM_RecommendedDisplayCIELabValue));

  return EC_Normal;
}


OFCondition DcmSegment::write(DcmItem& item)
{
  OFCondition result;
  result = m_SegmentDescription.write(item);
  DcmIODUtil::copyElementToDataset(result, item, m_SegmentAlgorithmName, m_Rules.getByTag(DCM_SegmentAlgorithmName));

  if (result.good() && m_SegmentSurfaceGenerationAlgorithmIdentification.check(OFTrue /* quiet */).good())
  {
    DcmIODUtil::writeSingleItem<AlgorithmIdentificationMacro>(
    result,
    DCM_SegmentSurfaceGenerationAlgorithmIdentificationSequence,
    m_SegmentSurfaceGenerationAlgorithmIdentification,
    item,
    "3",
    "Segmentation Image Module");
  }

  DcmIODUtil::copyElementToDataset(result, item, m_RecommendedDisplayGrayscaleValue, m_Rules.getByTag(DCM_RecommendedDisplayGrayscaleValue));
  DcmIODUtil::copyElementToDataset(result, item, m_RecommendedDisplayCIELabValue, m_Rules.getByTag(DCM_RecommendedDisplayCIELabValue));

  return result;
}



void DcmSegment::clearData()
{
  m_SegmentDescription.clearData();
  m_SegmentAlgorithmName.clear();
  m_SegmentSurfaceGenerationAlgorithmIdentification.clearData();
  m_RecommendedDisplayGrayscaleValue.clear();
  m_RecommendedDisplayCIELabValue.clear();
}



DcmSegment::~DcmSegment()
{
  clearData();
}


// protected default contstrucotr
DcmSegment::DcmSegment() :
  m_SegmentationDoc(NULL),
  m_SegmentDescription(),
  m_SegmentAlgorithmName(DCM_SegmentAlgorithmName),
  m_SegmentSurfaceGenerationAlgorithmIdentification(),
  m_RecommendedDisplayGrayscaleValue(DCM_RecommendedDisplayGrayscaleValue),
  m_RecommendedDisplayCIELabValue(DCM_RecommendedDisplayCIELabValue),
  m_Rules()
{
  initIODRules();
}


void DcmSegment::initIODRules()
{
  m_Rules.addRule(new IODRule(DCM_SegmentAlgorithmName, "1","1C","SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
  m_Rules.addRule(new IODRule(DCM_RecommendedDisplayGrayscaleValue, "1","3","SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
  m_Rules.addRule(new IODRule(DCM_RecommendedDisplayCIELabValue, "3","3","SegmentationImageModule", DcmIODTypes::IE_IMAGE), OFTrue);
}


// -------------- getters --------------------

Uint16 DcmSegment::getSegmentNumber()
{
  Uint16 value = 0;
  if (m_SegmentationDoc != NULL)
  {
    m_SegmentationDoc->getSegmentNumber(this, value);
  }
  return value;
}


OFCondition DcmSegment::getSegmentLabel(OFString& value,
                                        const signed long pos)
{
  return m_SegmentDescription.getSegmentLabel(value, pos);
}


OFCondition DcmSegment::getSegmentDescription(OFString& value,
                                              const signed long pos)
{
  return m_SegmentDescription.getSegmentDescription(value, pos);
}


DcmSegTypes::E_SegmentAlgoType DcmSegment::getSegmentAlgorithmType()
{
  return m_SegmentDescription.getSegmentAlgorithmType();
}


GeneralAnatomyMacro& DcmSegment::getGeneralAnatomyCode()
{
  return m_SegmentDescription.getGeneralAnatomyCode();
}


AlgorithmIdentificationMacro& DcmSegment::getSegmentSurfaceGenerationAlgorithmIdentification()
{
  return m_SegmentSurfaceGenerationAlgorithmIdentification;
}


CodeSequenceMacro& DcmSegment::getSegmentedPropertyCategoryCode()
{
  return m_SegmentDescription.getSegmentedPropertyCategoryCode();
}


CodeSequenceMacro& DcmSegment::getSegmentedPropertyTypeCode()
{
  return m_SegmentDescription.getSegmentedPropertyTypeCode();
}


OFVector< CodeSequenceMacro* >& DcmSegment::getSegmentedPropertyTypeModifierCode()
{
  return m_SegmentDescription.getSegmentedPropertyTypeModifier();
}



OFCondition DcmSegment::getRecommendedDisplayGrayscaleValue(Uint16& value,
                                                            const unsigned long pos)
{
  return m_RecommendedDisplayGrayscaleValue.getUint16(value, 0);
}


OFCondition DcmSegment::getRecommendedDisplayCIELabValue(Uint16& L,
                                                         Uint16& a,
                                                         Uint16& b)
{
  OFCondition result = m_RecommendedDisplayCIELabValue.getUint16(L, 0);
  if (result.good())
    result = m_RecommendedDisplayCIELabValue.getUint16(a, 1);

  if (result.good())
    result = m_RecommendedDisplayCIELabValue.getUint16(b, 2);

  return result;
}

// -------------- setters --------------------


OFCondition DcmSegment::setSegmentLabel(const OFString& value,
                                        const OFBool checkValue)
{
  return m_SegmentDescription.setSegmentLabel(value, checkValue);
}


OFCondition DcmSegment::setSegmentDescription(const OFString& value,
                                              const OFBool checkValue)
{
  return m_SegmentDescription.setSegmentDescription(value, checkValue);
}


OFCondition DcmSegment::setSegmentAlgorithm(const DcmSegTypes::E_SegmentAlgoType algoType,
                                            const OFString& algoName,
                                            const OFBool checkValue)
{
  if (checkValue && algoType == DcmSegTypes::SAT_UNKNOWN)
  {
    DCMSEG_ERROR("Algorithm type must be initialized to a valid value");
    return EC_InvalidValue;
  }

  OFCondition result;
  // Set algorithm name if algorithm type is not manual (otherwise do not set it at all)
  if (algoType != DcmSegTypes::SAT_MANUAL)
  {
    if ( checkValue && algoName.empty() )
    {
      DCMSEG_ERROR("Algorithm name must be provided if Algorithm Type is not 'MANUAL'");
      return EC_MissingValue;
    }
    if (!algoName.empty())
    {
      result = (checkValue) ? DcmLongString::checkStringValue(algoName, "1") : EC_Normal;
      if (result.good())
      {
        result = m_SegmentAlgorithmName.putOFStringArray(algoName);
      }
    }
  }
  // Set algorithm type
  if (result.good())
  {
   result =  m_SegmentDescription.setSegmentAlgorithmType(algoType);
  }
  return result;
}


OFCondition DcmSegment::setSegmentSurfaceGenerationAlgorithmIdentification(const AlgorithmIdentificationMacro& value,
                                                                           const OFBool checkValue)
{
  m_SegmentSurfaceGenerationAlgorithmIdentification = value;
  OFCondition result;
  if (checkValue)
  {
    result = m_SegmentSurfaceGenerationAlgorithmIdentification.check();
  }

  if (result.bad())
  {
    m_SegmentSurfaceGenerationAlgorithmIdentification.clearData();
  }

  return result;
}


OFCondition DcmSegment::setRecommendedDisplayGrayscaleValue(const Uint16 value,
                                                            const OFBool)
{
  return m_RecommendedDisplayGrayscaleValue.putUint16(value, 0);
}


OFCondition DcmSegment::setRecommendedDisplayCIELabValue(const Uint16 r,
                                                         const Uint16 g,
                                                         const Uint16 b,
                                                         const OFBool)
{
  OFCondition result = m_RecommendedDisplayCIELabValue.putUint16(r, 0);
  if (result.good())
    result = m_RecommendedDisplayCIELabValue.putUint16(g, 1);
  if (result.good())
    result = m_RecommendedDisplayCIELabValue.putUint16(b, 2);
  return result;
}



void DcmSegment::referenceSegmentationDoc(DcmSegmentation* doc)
{
  this->m_SegmentationDoc = doc;
}
