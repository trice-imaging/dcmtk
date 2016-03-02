/*
 *
 *  Copyright (C) 2015, J. Riesmeier, Oldenburg, Germany
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  Source file for class TID1600_ImageLibrary
 *
 *  Author: Joerg Riesmeier
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/cmr/tid1600.h"
#include "dcmtk/dcmsr/cmr/cid29e.h"
#include "dcmtk/dcmsr/cmr/cid244e.h"
#include "dcmtk/dcmsr/cmr/cid4031e.h"
#include "dcmtk/dcmsr/cmr/cid10013e.h"
#include "dcmtk/dcmsr/cmr/cid10033e.h"
#include "dcmtk/dcmsr/cmr/logger.h"
#include "dcmtk/dcmsr/codes/dcm.h"
#include "dcmtk/dcmsr/codes/srt.h"
#include "dcmtk/dcmsr/codes/ucum.h"

#include "dcmtk/dcmsr/dsrdattn.h"
#include "dcmtk/dcmsr/dsrnumvl.h"


// helper macros for checking the return value of API calls
#define CHECK_RESULT(call) if (result.good()) result = call
#define STORE_RESULT(call) result = call
#define DELETE_ERROR(pointer) if (result.bad()) delete pointer

// index positions in node list (makes source code more readable)
#define LAST_IMAGE_LIBRARY_GROUP 0
#define LAST_IMAGE_LIBRARY_ENTRY 1

// general information on TID 1600 (Image Library)
#define TEMPLATE_NUMBER      "1600"
#define MAPPING_RESOURCE     "DCMR"
#define MAPPING_RESOURCE_UID UID_DICOMContentMappingResource
#define TEMPLATE_TYPE        OFTrue  /* extensible */


// conditions constants
makeOFConditionConst(CMR_EC_NoImageLibraryGroup,                           OFM_dcmsr, 1600, OF_error, "No Image Library Group");
makeOFConditionConst(CMR_EC_CannotAddMultipleImageLibraryEntryDescriptors, OFM_dcmsr, 1602, OF_error, "Cannot add multiple Image Library Entry Descriptors");


TID1600_ImageLibrary::TID1600_ImageLibrary()
  : DSRSubTemplate(TEMPLATE_NUMBER, MAPPING_RESOURCE, MAPPING_RESOURCE_UID)
{
    setExtensible(TEMPLATE_TYPE);
    /* need to store last image library group and entry */
    reserveEntriesInNodeList(2);
    /* TID 1600 (Image Library) Row 1 */
    if (addChildContentItem(RT_unknown, VT_Container, CODE_DCM_ImageLibrary).good())
        getCurrentContentItem().setAnnotationText("TID 1600 - Row 1");
}


OFCondition TID1600_ImageLibrary::addImageGroup()
{
    OFCondition result = EC_Normal;
    /* go to last image library group (if any) */
    if (gotoEntryFromNodeList(this, LAST_IMAGE_LIBRARY_GROUP) > 0)
    {
        /* TID 1600 (Image Library) Row 2, append to last group */
        STORE_RESULT(addContentItem(RT_contains, VT_Container, CODE_DCM_ImageLibraryGroup));
    } else {
        /* TID 1600 (Image Library) Row 2, create first group (child) */
        STORE_RESULT(addChildContentItem(RT_contains, VT_Container, CODE_DCM_ImageLibraryGroup));
    }
    CHECK_RESULT(getCurrentContentItem().setAnnotationText("TID 1600 - Row 2"));
    /* store ID of recently added node for later use */
    if (result.good())
    {
        storeEntryInNodeList(LAST_IMAGE_LIBRARY_GROUP, getNodeID());
        storeEntryInNodeList(LAST_IMAGE_LIBRARY_ENTRY, 0 /* forget last entry */);
    }
    return result;
}



OFCondition TID1600_ImageLibrary::addImageEntry(DcmItem &dataset,
                                                const AddImageMode mode,
                                                const OFBool check)
{
    OFCondition result = EC_MemoryExhausted;
    /* create a new subtree in order to "rollback" in case of error */
    DSRDocumentSubTree *tid1601 = new DSRDocumentSubTree;
    if (tid1601 != NULL)
    {
        DSRImageReferenceValue imageRef;
        /* TID 1601 (Image Library Entry) Row 1 */
        STORE_RESULT(tid1601->addContentItem(RT_contains, VT_Image, DSRCodedEntryValue()));
        CHECK_RESULT(imageRef.setReference(dataset, check));
        CHECK_RESULT(tid1601->getCurrentContentItem().setImageReference(imageRef, check));
        CHECK_RESULT(tid1601->getCurrentContentItem().setAnnotationText("TID 1601 - Row 1"));
        const size_t lastNode = tid1601->getNodeID();
        /* TID 1601 (Image Library Entry) Row 2 */
        if (mode == withAllDescriptors)
        {
            /* create a new subtree for TID 1602 (Image Library Entry Descriptors) */
            DSRDocumentSubTree *tid1602 = new DSRDocumentSubTree;
            if (tid1602 != NULL)
            {
                /* call the function doing the real work */
                result = addImageEntryDescriptors(*tid1602, dataset, check);
                /* if everything was OK, insert new subtree into the template */
                if (result.good() && !tid1602->isEmpty())
                {
                    /* insert subtree below current node */
                    result = tid1601->insertSubTree(tid1602);
                }
                /* in case of error, make sure that memory is freed */
                DELETE_ERROR(tid1602);
            } else
                result = EC_MemoryExhausted;
        }
        /* if everything was OK, insert new subtree into the template */
        if (result.good())
        {
            E_AddMode addMode = AM_afterCurrent;
            /* go to last image library entry (if any) */
            if (gotoEntryFromNodeList(this, LAST_IMAGE_LIBRARY_ENTRY) > 0)
                addMode = AM_afterCurrent;
            /* go to last image library group (if any) */
            else if (gotoEntryFromNodeList(this, LAST_IMAGE_LIBRARY_GROUP) > 0)
                addMode = AM_belowCurrent;
            else
                result = CMR_EC_NoImageLibraryGroup;
            /* there is at least an image library group */
            if (result.good())
            {
                /* insert subtree at current position */
                result = insertSubTree(tid1601, addMode);
                /* store ID of recently added node for later use */
                if (result.good())
                    storeEntryInNodeList(LAST_IMAGE_LIBRARY_ENTRY, lastNode);
            }
        }
        /* in case of error, make sure that memory is freed */
        DELETE_ERROR(tid1601);
    }
    return result;
}


OFCondition TID1600_ImageLibrary::addImageEntryDescriptors(DcmItem &dataset,
                                                           const OFBool check)
{
    OFCondition result = EC_MemoryExhausted;
    /* create a new subtree in order to "rollback" in case of error */
    DSRDocumentSubTree *subTree = new DSRDocumentSubTree;
    if (subTree != NULL)
    {
        /* call the function doing the real work */
        result = addImageEntryDescriptors(*subTree, dataset, check);
        /* if everything was OK, insert new subtree into the template */
        if (result.good() && !subTree->isEmpty())
        {
            /* go to last image library group (if any) */
            if (gotoLastEntryFromNodeList(this, LAST_IMAGE_LIBRARY_GROUP) > 0)
            {

                /* check whether TID 1600 (Image Library) Row 3 is already there */
                const DSRDocumentTreeNode *childNode = getChildNode();
                if ((childNode != NULL) && (childNode->getRelationshipType() == RT_hasAcqContext))
                {
                    /* only a single invocation of the included template allowed */
                    result = CMR_EC_CannotAddMultipleImageLibraryEntryDescriptors;
                } else {
                    /* insert subtree at current position */
                    result = insertSubTree(subTree, AM_belowCurrentBeforeFirstChild);
                }
            } else
                result = CMR_EC_NoImageLibraryGroup;
        }
        /* in case of error, make sure that memory is freed */
        DELETE_ERROR(subTree);
    }
    return result;
}


// protected methods

OFCondition TID1600_ImageLibrary::addImageEntryDescriptors(DSRDocumentSubTree &tree,
                                                           DcmItem &dataset,
                                                           const OFBool check)
{
    OFCondition result = EC_Normal;
    /* TID 1602 (Image Library Entry Descriptors) Row 1 */
    OFString modality;
    if (getStringValueFromDataset(dataset, DCM_Modality, modality).good() && !modality.empty())
    {
        /* determine modality code from CID 29 */
        const CID29e_AcquisitionModality contextGroup;
        DSRCodedEntryValue modalityCode(contextGroup.mapModality(modality));
        if (modalityCode.isValid())
        {
            CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Code, CODE_DCM_Modality));
            CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(modalityCode, check));
            CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1602 - Row 1"));
        } else {
            /* do not treat this as an error, just report a warning */
            DCMSR_CMR_WARN("Cannot map Modality '" << modality << "' to a coded entry (not in CID 29)");
        }
    }
    /* TID 1602 (Image Library Entry Descriptors) Row 2 */
    DSRCodedEntryValue regionCode;
    /* try to get coded entry from code sequence */
    if (regionCode.readSequence(dataset, DCM_AnatomicRegionSequence, "3" /*type*/).bad())
    {
        OFString bodyPartExamined;
        if (getStringValueFromDataset(dataset, DCM_BodyPartExamined, bodyPartExamined).good() && !bodyPartExamined.empty())
        {
            /* alternatively, determine target region code from CID 4031 (using PS 3.16 Annex L) */
            regionCode = CID4031e_CommonAnatomicRegions::mapBodyPartExamined(bodyPartExamined);
            if (!regionCode.isValid())
            {
                /* report this as a debug message (avoid too many warnings) */
                DCMSR_CMR_DEBUG("Cannot map Body Part Examined '" << bodyPartExamined << "' to a coded entry (no mapping to CID 4031 defined)");
            }
        }
    }
    if (regionCode.isValid())
    {
        CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Code, CODE_DCM_TargetRegion));
        CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(regionCode, check));
        CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1602 - Row 2"));
    }
    /* TID 1602 (Image Library Entry Descriptors) Row 3 */
    OFString imageLaterality;
    if (getStringValueFromDataset(dataset, DCM_ImageLaterality, imageLaterality).good() && !imageLaterality.empty())
    {
        /* determine image laterality code from CID 244 */
        DSRCodedEntryValue lateralityCode(CID244e_Laterality::mapImageLaterality(imageLaterality));
        if (lateralityCode.isValid())
        {
            CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Code, CODE_DCM_ImageLaterality));
            CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(lateralityCode, check));
            CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1602 - Row 3"));
        } else {
            /* do not treat this as an error, just report a warning */
            DCMSR_CMR_WARN("Cannot map Image Laterality '" << imageLaterality << "' to a coded entry (not in CID 244)");
        }
    }
    /* TID 1602 (Image Library Entry Descriptors) Row 4 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_StudyDate, 0 /*pos*/, VT_Date, CODE_DCM_StudyDate, "TID 1602 - Row 4", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 5 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_StudyTime, 0 /*pos*/, VT_Time, CODE_DCM_StudyTime, "TID 1602 - Row 5", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 6 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_ContentDate, 0 /*pos*/, VT_Date, CODE_DCM_ContentDate, "TID 1602 - Row 6", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 7 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_ContentTime, 0 /*pos*/, VT_Time, CODE_DCM_ContentTime, "TID 1602 - Row 7", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 8 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_AcquisitionDate, 0 /*pos*/, VT_Date, CODE_DCM_AcquisitionDate, "TID 1602 - Row 8", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 9 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_AcquisitionTime, 0 /*pos*/, VT_Time, CODE_DCM_AcquisitionTime, "TID 1602 - Row 9", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 10 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_FrameOfReferenceUID, 0 /*pos*/, VT_UIDRef, CODE_DCM_FrameOfReferenceUID, "TID 1602 - Row 10", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 11 */
    /* - tbc: what about DCM_TotalPixelMatrixRows (e.g. used for WSI images)? */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_Rows, 0 /*pos*/, CODE_DCM_PixelDataRows, CODE_UCUM_Pixels, "TID 1602 - Row 11", check));
    /* TID 1602 (Image Library Entry Descriptors) Row 12 */
    /* - tbc: what about DCM_TotalPixelMatrixColumns (e.g. used for WSI images)? */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_Columns, 0 /*pos*/, CODE_DCM_PixelDataColumns, CODE_UCUM_Pixels, "TID 1602 - Row 12", check));
    /* continue with modality-specific descriptors (TID 1603 to 1607) */
    CHECK_RESULT(addModalitySpecificDescriptors(tree, dataset, modality, check));
    return result;
}


OFCondition TID1600_ImageLibrary::addModalitySpecificDescriptors(DSRDocumentSubTree &tree,
                                                                 DcmItem &dataset,
                                                                 const OFString &modality,
                                                                 const OFBool check)
{
    OFCondition result = EC_Normal;
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) */
    if ((modality == "CR") || (modality == "RG") || (modality == "DX") || (modality == "MG") || (modality == "XA") || (modality == "RF") || (modality == "PX") || (modality == "IO"))
        CHECK_RESULT(addProjectionRadiographyDescriptors(tree, dataset, check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) */
    if ((modality == "CT") || (modality == "MR") || (modality == "US") /* correct? */ || (modality == "PT") || (modality == "OCT") || (modality == "OPT") || (modality == "IVOCT"))
        CHECK_RESULT(addCrossSectionalModalitiesDescriptors(tree, dataset, check));
    /* TID 1605 (Image Library Entry Descriptors for CT) */
    if (modality == "CT")
        CHECK_RESULT(addComputedTomographyDescriptors(tree, dataset, check));
    /* TID 1606 (Image Library Entry Descriptors for MR) */
    if (modality == "MR")
        CHECK_RESULT(addMagneticResonanceDescriptors(tree, dataset, check));
    /* TID 1607 (Image Library Entry Descriptors for PET) */
    if (modality == "PT")
        CHECK_RESULT(addPositronEmissionTomographyDescriptors(tree, dataset, check));
    return result;
}


OFCondition TID1600_ImageLibrary::addProjectionRadiographyDescriptors(DSRDocumentSubTree &tree,
                                                                      DcmItem &dataset,
                                                                      const OFBool check)
{
    OFCondition result = EC_Normal;
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 1 */
    CHECK_RESULT(addCodeContentItemFromDataset(tree, dataset, DCM_ViewCodeSequence, CODE_DCM_ImageView, "TID 1603 - Row 1", check));
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 2 */
    if (result.good() && (tree.getCurrentContentItem().getConceptName() == CODE_DCM_ImageView))
    {
        DcmItem *item = NULL;
        /* get view modifiers (if any) */
        if (dataset.findAndGetSequenceItem(DCM_ViewCodeSequence, item, 0 /*itemNum*/).good())
        {
            DcmSequenceOfItems *sequence = NULL;
            if (item->findAndGetSequence(DCM_ViewModifierCodeSequence, sequence).good())
            {
                /* iterate over all sequence items */
                DcmObject *object = NULL;
                while (((object = sequence->nextInContainer(object)) != NULL) && result.good())
                {
                    DSRCodedEntryValue modifierCode;
                    if (modifierCode.readSequenceItem(*OFstatic_cast(DcmItem *, object), DCM_ViewModifierCodeSequence).good())
                    {
                        CHECK_RESULT(tree.addChildContentItem(RT_hasAcqContext, VT_Code, CODE_DCM_ImageViewModifier));
                        CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(modifierCode, check));
                        CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1603 - Row 2"));
                        tree.goUp();
                    }
                }
            }
        }
    }
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 3 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_PatientOrientation, 0 /*pos*/, VT_Text, CODE_DCM_PatientOrientationRow, "TID 1603 - Row 3", check));
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 4 */
    CHECK_RESULT(addStringContentItemFromDataset(tree, dataset, DCM_PatientOrientation, 1 /*pos*/, VT_Text, CODE_DCM_PatientOrientationColumn, "TID 1603 - Row 4", check));
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 5 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImagerPixelSpacing, 1 /*pos*/, CODE_DCM_HorizontalPixelSpacing, CODE_UCUM_Millimeter, "TID 1603 - Row 5", check));
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 6 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImagerPixelSpacing, 0 /*pos*/, CODE_DCM_VerticalPixelSpacing, CODE_UCUM_Millimeter, "TID 1603 - Row 6", check));
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 7 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_PositionerPrimaryAngle, 0 /*pos*/, CODE_DCM_PositionerPrimaryAngle, CODE_UCUM_Degrees, "TID 1603 - Row 7", check));
    /* TID 1603 (Image Library Entry Descriptors for Projection Radiography) Row 8 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_PositionerSecondaryAngle, 0 /*pos*/, CODE_DCM_PositionerSecondaryAngle, CODE_UCUM_Degrees, "TID 1603 - Row 8", check));
    return result;
}


OFCondition TID1600_ImageLibrary::addCrossSectionalModalitiesDescriptors(DSRDocumentSubTree &tree,
                                                                         DcmItem &dataset,
                                                                         const OFBool check)
{
    OFCondition result = EC_Normal;
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 1 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_PixelSpacing, 1 /*pos*/, CODE_DCM_HorizontalPixelSpacing, CODE_UCUM_Millimeter, "TID 1604 - Row 1", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 2 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_PixelSpacing, 0 /*pos*/, CODE_DCM_VerticalPixelSpacing, CODE_UCUM_Millimeter, "TID 1604 - Row 2", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 3 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_SpacingBetweenSlices, 0 /*pos*/, CODE_DCM_SpacingBetweenSlices, CODE_UCUM_Millimeter, "TID 1604 - Row 3", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 4 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_SliceThickness, 0 /*pos*/, CODE_DCM_SliceThickness, CODE_UCUM_Millimeter, "TID 1604 - Row 4", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 5 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImagePositionPatient, 0 /*pos*/, CODE_DCM_ImagePosition_Patient_X, CODE_UCUM_Millimeter, "TID 1604 - Row 5", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 6 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImagePositionPatient, 1 /*pos*/, CODE_DCM_ImagePosition_Patient_Y, CODE_UCUM_Millimeter, "TID 1604 - Row 6", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 7 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImagePositionPatient, 2 /*pos*/, CODE_DCM_ImagePosition_Patient_Z, CODE_UCUM_Millimeter, "TID 1604 - Row 7", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 8 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImageOrientationPatient, 0 /*pos*/, CODE_DCM_ImageOrientation_Patient_RowX, CODE_UCUM_Minus1To1, "TID 1604 - Row 8", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 9 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImageOrientationPatient, 1 /*pos*/, CODE_DCM_ImageOrientation_Patient_RowY, CODE_UCUM_Minus1To1, "TID 1604 - Row 9", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 10 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImageOrientationPatient, 2 /*pos*/, CODE_DCM_ImageOrientation_Patient_RowZ, CODE_UCUM_Minus1To1, "TID 1604 - Row 10", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 11 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImageOrientationPatient, 3 /*pos*/, CODE_DCM_ImageOrientation_Patient_ColumnX, CODE_UCUM_Minus1To1, "TID 1604 - Row 11", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 12 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImageOrientationPatient, 4 /*pos*/, CODE_DCM_ImageOrientation_Patient_ColumnY, CODE_UCUM_Minus1To1, "TID 1604 - Row 12", check));
    /* TID 1604 (Image Library Entry Descriptors for Cross-Sectional Modalities) Row 13 */
    CHECK_RESULT(addNumericContentItemFromDataset(tree, dataset, DCM_ImageOrientationPatient, 5 /*pos*/, CODE_DCM_ImageOrientation_Patient_ColumnZ, CODE_UCUM_Minus1To1, "TID 1604 - Row 13", check));
    return result;
}


OFCondition TID1600_ImageLibrary::addComputedTomographyDescriptors(DSRDocumentSubTree &tree,
                                                                   DcmItem &dataset,
                                                                   const OFBool check)
{
    OFCondition result = EC_Normal;
    /* TID 1605 (Image Library Entry Descriptors for CT) Row 1 */
    DcmSequenceOfItems *ctAcquisitionTypeSequence = NULL;
    /* - tbd: only check in functional groups sequences? */
    if (dataset.findAndGetSequence(DCM_CTAcquisitionTypeSequence, ctAcquisitionTypeSequence, OFTrue /*searchIntoSub*/).good())
    {
        DcmItem *item = ctAcquisitionTypeSequence->getItem(0);
        if (item != NULL)
        {
            OFString acquisitionType;
            if (getStringValueFromDataset(*item, DCM_AcquisitionType, acquisitionType).good() && !acquisitionType.empty())
            {
                /* determine CT acquisition type code from CID 10013 */
                DSRCodedEntryValue acquisitionTypeCode(CID10013e_CTAcquisitionType::mapAcquisitionType(acquisitionType));
                if (acquisitionTypeCode.isValid())
                {
                    CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Code, CODE_DCM_CTAcquisitionType));
                    CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(acquisitionTypeCode, check));
                    CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1605 - Row 1"));
                } else {
                    /* do not treat this as an error, just report a warning */
                    DCMSR_CMR_WARN("Cannot map Acquisition Type '" << acquisitionType << "' to a coded entry (not in CID 10013)");
                }
            }
        }
    }
    /* TID 1605 (Image Library Entry Descriptors for CT) Row 2 */
    DcmSequenceOfItems *ctReconstructionSequence = NULL;
    /* - tbd: only check in functional groups sequences? */
    if (dataset.findAndGetSequence(DCM_CTReconstructionSequence, ctReconstructionSequence, OFTrue /*searchIntoSub*/).good())
    {
        DcmItem *item = ctReconstructionSequence->getItem(0);
        if (item != NULL)
        {
            OFString reconstructionAlgorithm;
            if (getStringValueFromDataset(*item, DCM_ReconstructionAlgorithm, reconstructionAlgorithm).good() && !reconstructionAlgorithm.empty())
            {
                /* determine CT reconstruction algorithm code from CID 10033 */
                DSRCodedEntryValue reconstructionAlgorithmCode(CID10033e_CTReconstructionAlgorithm::mapReconstructionAlgorithm(reconstructionAlgorithm));
                if (reconstructionAlgorithmCode.isValid())
                {
                    CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Code, CODE_DCM_ReconstructionAlgorithm));
                    CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(reconstructionAlgorithmCode, check));
                    CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1605 - Row 2"));
                } else {
                    /* do not treat this as an error, just report a warning */
                    DCMSR_CMR_WARN("Cannot map Reconstruction Algorithm '" << reconstructionAlgorithm << "' to a coded entry (not in CID 10033)");
                }
            }
        }
    }
    return result;
}


OFCondition TID1600_ImageLibrary::addMagneticResonanceDescriptors(DSRDocumentSubTree &tree,
                                                                  DcmItem &dataset,
                                                                  const OFBool check)
{
    OFCondition result = EC_Normal;
    /* TID 1606 (Image Library Entry Descriptors for MR) Row 1 */
    OFString sequenceName;
    /* get one of two alternative elements values */
    if ((getStringValueFromDataset(dataset, DCM_PulseSequenceName, sequenceName).good() && !sequenceName.empty()) ||
        (getStringValueFromDataset(dataset, DCM_SequenceName, sequenceName).good() && !sequenceName.empty()))
    {
        CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Text, DSRCodedEntryValue("110909", "DCM", "Pulse Sequence Name") /* wrong definition? CODE_DCM_PulseSequenceName */));
        CHECK_RESULT(tree.getCurrentContentItem().setStringValue(sequenceName, check));
        CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText("TID 1606 - Row 1"));
    }
    return result;
}


OFCondition TID1600_ImageLibrary::addPositronEmissionTomographyDescriptors(DSRDocumentSubTree &tree,
                                                                           DcmItem &dataset,
                                                                           const OFBool check)
{
    OFCondition result = EC_Normal;
    /* get main sequence from (Enhanced) PET Isotope Module */
    DcmSequenceOfItems *radiopharmaceuticalInformationSequence = NULL;
    if (dataset.findAndGetSequence(DCM_RadiopharmaceuticalInformationSequence, radiopharmaceuticalInformationSequence, OFTrue /*searchIntoSub*/).good())
    {
        /* tbd: sequence may contain multiple items! */
        DcmItem *item = radiopharmaceuticalInformationSequence->getItem(0);
        if (item != NULL)
        {
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 1 */
            CHECK_RESULT(addCodeContentItemFromDataset(tree, *item, DCM_RadionuclideCodeSequence, CODE_SRT_Radionuclide, "TID 1607 - Row 1", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 2 */
            CHECK_RESULT(addCodeContentItemFromDataset(tree, *item, DCM_RadiopharmaceuticalCodeSequence, CODE_SRT_RadiopharmaceuticalAgent, "TID 1607 - Row 2", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 3 */
            CHECK_RESULT(addNumericContentItemFromDataset(tree, *item, DCM_RadionuclideHalfLife, 0 /*pos*/, CODE_SRT_HalfLifeOfRadiopharmaceutical, CODE_UCUM_S, "TID 1607 - Row 3", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 4 */
            CHECK_RESULT(addStringContentItemFromDataset(tree, *item, DCM_RadiopharmaceuticalStartDateTime, 0 /*pos*/, VT_DateTime, CODE_DCM_RadiopharmaceuticalStartTime /*DateTime*/, "TID 1607 - Row 4", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 4b */
            CHECK_RESULT(addStringContentItemFromDataset(tree, *item, DCM_RadiopharmaceuticalStopDateTime, 0 /*pos*/, VT_DateTime, CODE_DCM_RadiopharmaceuticalStopTime /*DateTime*/, "TID 1607 - Row 4b", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 5 */
            CHECK_RESULT(addNumericContentItemFromDataset(tree, *item, DCM_RadiopharmaceuticalVolume, 0 /*pos*/, CODE_DCM_RadiopharmaceuticalVolume, CODE_UCUM_Cm3, "TID 1607 - Row 5", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 6 */
            CHECK_RESULT(addNumericContentItemFromDataset(tree, *item, DCM_RadionuclideTotalDose, 0 /*pos*/, CODE_DCM_RadionuclideTotalDose, CODE_UCUM_Bq, "TID 1607 - Row 6", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 7 */
            CHECK_RESULT(addNumericContentItemFromDataset(tree, *item, DCM_RadiopharmaceuticalSpecificActivity, 0 /*pos*/, CODE_DCM_RadiopharmaceuticalSpecificActivity, CODE_UCUM_BqPerMol, "TID 1607 - Row 7", check));
            /* TID 1607 (Image Library Entry Descriptors for PET) Row 8 */
            CHECK_RESULT(addCodeContentItemFromDataset(tree, *item, DCM_AdministrationRouteCodeSequence, CODE_SRT_RouteOfAdministration, "TID 1607 - Row 8", check));
        }
    }
    /* TID 1607 (Image Library Entry Descriptors for PET) Row 9 to 10
     * - contained in TID 15101 (NM/PET Protocol Context), i.e. not available in the image
     */
    /* TID 1607 (Image Library Entry Descriptors for PET) Row 11
     * - tbc: where to get "Radionuclide Incubation Time
     */
    /* TID 1607 (Image Library Entry Descriptors for PET) Row 12 to 14
     * - contained in TID 15101 (NM/PET Protocol Context), i.e. available from Modality Worklist, or
     * - tbd: in TID 3470 (NM/PET Acquisition Context), i.e. from the Acquisition Context Module
     */
    return result;
}


// static helper functions

OFCondition TID1600_ImageLibrary::addStringContentItemFromDataset(DSRDocumentSubTree &tree,
                                                                  DcmItem &dataset,
                                                                  const DcmTagKey &tagKey,
                                                                  const signed long pos,
                                                                  const E_ValueType valueType,
                                                                  const DSRCodedEntryValue &conceptName,
                                                                  const OFString &annotationText,
                                                                  const OFBool check)
{
    OFCondition result = EC_Normal;
    OFString stringValue;
    /* get element value from dataset (textual content only) */
    if (getStringValueFromDataset(dataset, tagKey, stringValue, pos).good() && !stringValue.empty())
    {
        /* create new content item, set concept name and value */
        CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, valueType, conceptName));
        CHECK_RESULT(tree.getCurrentContentItem().setStringValue(stringValue, check));
        if (!annotationText.empty())
            CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText(annotationText));
    }
    return result;
}


OFCondition TID1600_ImageLibrary::addCodeContentItemFromDataset(DSRDocumentSubTree &tree,
                                                                DcmItem &dataset,
                                                                const DcmTagKey &tagKey,
                                                                const DSRCodedEntryValue &conceptName,
                                                                const OFString &annotationText,
                                                                const OFBool check)
{
    OFCondition result = EC_Normal;
    DSRCodedEntryValue codedEntry;
    /* get coded entry from code sequence in dataset */
    if (codedEntry.readSequence(dataset, tagKey, "3" /*type*/).good() && codedEntry.isValid())
    {
        CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Code, conceptName));
        CHECK_RESULT(tree.getCurrentContentItem().setCodeValue(codedEntry, check));
        if (!annotationText.empty())
            CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText(annotationText));
    }
    return result;
}


OFCondition TID1600_ImageLibrary::addNumericContentItemFromDataset(DSRDocumentSubTree &tree,
                                                                   DcmItem &dataset,
                                                                   const DcmTagKey &tagKey,
                                                                   const signed long pos,
                                                                   const DSRCodedEntryValue &conceptName,
                                                                   const DSRCodedEntryValue &measurementUnit,
                                                                   const OFString &annotationText,
                                                                   const OFBool check)
{
    OFCondition result = EC_Normal;
    OFString numericValue;
    /* get element value from dataset (in text format) */
    if (getStringValueFromDataset(dataset, tagKey, numericValue, pos).good() && !numericValue.empty())
    {
        /* create new content item, set concept name and value */
        CHECK_RESULT(tree.addContentItem(RT_hasAcqContext, VT_Num, conceptName));
        CHECK_RESULT(tree.getCurrentContentItem().setNumericValue(DSRNumericMeasurementValue(numericValue, measurementUnit), check));
        if (!annotationText.empty())
            CHECK_RESULT(tree.getCurrentContentItem().setAnnotationText(annotationText));
    }
    return result;
}
