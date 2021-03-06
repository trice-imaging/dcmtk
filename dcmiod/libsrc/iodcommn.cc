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
 *  Module: dcmiod
 *
 *  Author: Michael Onken
 *
 *  Purpose: Class representing IODs by exposing common DICOM module attributes
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmiod/iodcommn.h"
#include "dcmtk/dcmdata/dctypes.h"    // logger
#include "dcmtk/dcmiod/iodutil.h"


DcmIODCommon::DcmIODCommon()
: m_Item(new DcmItem()),
  m_Rules(new IODRules),
  m_Patient(m_Item, m_Rules),
  m_PatientStudy(m_Item, m_Rules),
  m_Study(m_Item, m_Rules),
  m_Equipment(m_Item, m_Rules),
  m_Series(m_Item, m_Rules),
  m_FrameOfReference(m_Item, m_Rules),
  m_SOPCommon(m_Item, m_Rules),
  m_CommonInstanceReferenceModule(m_Item, m_Rules),
  m_Modules()
{
  // Set initial values for a new SOP instance
  ensureInstanceUIDs(OFFalse);
  m_Modules.push_back(&m_Patient);
  m_Modules.push_back(&m_PatientStudy);
  m_Modules.push_back(&m_Study);
  m_Modules.push_back(&m_Equipment);
  m_Modules.push_back(&m_Series);
  m_Modules.push_back(&m_FrameOfReference);
  m_Modules.push_back(&m_SOPCommon);
  m_Modules.push_back(&m_CommonInstanceReferenceModule);
}


DcmIODCommon::~DcmIODCommon()
{
}


void DcmIODCommon::clearData()
{
  OFVector<IODModule*>::iterator it = m_Modules.begin();
  while (it != m_Modules.end())
  {
    (*it)->clearData();
    it++;
  }
}


IODPatientModule& DcmIODCommon::getPatient()
{
  return m_Patient;
}


IODPatientStudyModule& DcmIODCommon::getPatientStudy()
{
  return m_PatientStudy;
}


IODGeneralStudyModule& DcmIODCommon::getStudy()
{
  return m_Study;
}


IODGeneralEquipmentModule& DcmIODCommon::getEquipment()
{
  return m_Equipment;
}


IODGeneralSeriesModule& DcmIODCommon::getSeries()
{
  return m_Series;
}


IODFoRModule& DcmIODCommon::getFrameOfReference()
{
  return m_FrameOfReference;
}


IODSOPCommonModule& DcmIODCommon::getSOPCommon()
{
  return m_SOPCommon;
}


IODCommonInstanceReferenceModule& DcmIODCommon::getCommonInstanceReference()
{
  return m_CommonInstanceReferenceModule;
}



OFshared_ptr<IODRules> DcmIODCommon::getRules()
{
  return m_Rules;
}


OFshared_ptr<DcmItem> DcmIODCommon::getData()
{
  return m_Item;
}


OFCondition DcmIODCommon::read(DcmItem &dataset)
{
  /* re-initialize object */
  DcmIODCommon::clearData();

  OFVector<IODModule*>::iterator it = m_Modules.begin();
  while ( it != m_Modules.end() )
  {
    (*it)->read(dataset, OFTrue /* clear old data */);
    it++;
  }

  // we do not report errors here (only logger output)
  return EC_Normal;
}


OFCondition DcmIODCommon::import(DcmItem& dataset,
                                 OFBool readPatient,
                                 OFBool readStudy,
                                 OFBool readSeries,
                                 OFBool readFoR)
{
  if (readPatient)
  {
    m_Patient.read(dataset, OFFalse /* do not clear old data */);
  }

  if (readStudy)
  {
    m_Study.read(dataset, OFFalse /* do not clear old data */);
    m_Equipment.read(dataset, OFFalse /* do not clear old data */);
    m_PatientStudy.read(dataset, OFFalse /* do not clear old data */);
  }

  if (readSeries)
  {
    m_Series.read(dataset, OFFalse /* do not clear old data */);
    m_FrameOfReference.read(dataset, OFFalse /* do not clear old data */);
  }

  if (readFoR)
  {
    m_FrameOfReference.read(dataset, OFFalse /* do not clear old data */);
  }

  return EC_Normal;
}


void DcmIODCommon::ensureInstanceUIDs(const OFBool correctInvalid)
{
  m_Study.ensureInstanceUID(correctInvalid);
  m_Series.ensureInstanceUID(correctInvalid);
  m_SOPCommon.ensureInstanceUID(correctInvalid);
}


OFCondition DcmIODCommon::write(DcmItem &dataset)
{
  OFCondition result;

  OFVector<IODModule*>::iterator it = m_Modules.begin();
  while ( (it != m_Modules.end() && result.good()) )
  {
    result = (*it)->write(dataset);
    it++;
  }
  return result;
}


void DcmIODCommon::createNewStudy(const OFBool clearEquipment)
{
  // clear all study-related attributes
  m_Study.clearData();
  m_PatientStudy.clearData();
  if (clearEquipment)
    m_Equipment.clearData();
  // make sure we have a valid Study Instance UID
  m_Study.ensureInstanceUID();

  // reset series- and instance related attributes
  createNewSeries();
}


void DcmIODCommon::createNewSeries(const OFBool clearFoR)
{
  // clear all series-related attributes
  m_Series.clearData();
  // create new Series Instance UID
  m_Series.ensureInstanceUID();

  // clear frame of reference-related attributes if desired
  if (clearFoR)
    m_FrameOfReference.clearData();

  /* also creates new series (since UID is empty) */
  createNewSOPInstance();
}


void DcmIODCommon::createNewSOPInstance()
{
  m_SOPCommon.clearData();
  m_SOPCommon.ensureInstanceUID();
}


