#ifndef _KIS_SIOX_SEGMENTATOR_H_
#define _KIS_SIOX_SEGMENTATOR_H_

#include <QHash>
#include <QString>

#include <boost/tuple/tuple.hpp>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kundo2command.h>

#include "kisColorSignature.h"
#include "kis_types.h"

//#if 0
/**
  SIOX image segmentator.

  http://www.siox.org/

  TODO
  - Document the class
  - Document the functions
  - Change confidence matrix input to a krita type
  - Implement the distance calculation function
  - Find a relation between quint16 and float Lab types
 */
class KisSioxSegmentator
{
private:
    /**
      ClusterDistance tuple indexes.
     */
    enum ClusterDistanceIndexes {
        MIN_BG_DIST = 0,
        MIN_BG_INDX = 1,
        MIN_FG_DIST = 2,
        MIN_FG_INDX = 3
    };

    typedef boost::tuple< float, int, float, int > ClusterDistance;

    typedef QHash< const quint16*, ClusterDistance > NearestPixelsMap;

public:
    /**
      Confidence corresponding to a certain foreground region (equals one).
     */
    static const float CERTAIN_FOREGROUND_CONFIDENCE = 1.0f;

    /**
      Confidence for a region likely being foreground.
     */
    static const float FOREGROUND_CONFIDENCE = 0.8f;

    /**
      Confidence for foreground or background type being equally likely.
     */
    static const float UNKNOWN_REGION_CONFIDENCE = 0.5f;

    /**
      Confidence for a region likely being background.
     */
    static const float BACKGROUND_CONFIDENCE = 0.1f;

    /**
      Confidence corresponding to a certain background reagion (equals zero).
     */
    static const float CERTAIN_BACKGROUND_CONFIDENCE = 0.0f;

    /**
      Default values for cluster size in Lab axis.
     */
    static const float L_DEFAULT_CLUSTER_SIZE = 0.64f;
    static const float A_DEFAULT_CLUSTER_SIZE = 1.28f;
    static const float B_DEFAULT_CLUSTER_SIZE = 2.56f;

private:
    /**
      The image to be segmentated.
     */
    KisPaintDeviceSP image;

    /**
      The image converted to Lab colorspace.
     */
    KisPaintDeviceSP labImage;

    /**
      Stores component label (index) by pixel it belongs to.
     */
    QVector< qint64 > labelField;

    /**
      Holds background signature (a characteristic subset of the background).
     */
    ColorSignature< const quint16* >::Type bgSignature;

    /**
      Holds foreground signature (a characteristic subset of the foreground).
     */
    ColorSignature< const quint16* >::Type fgSignature;

    /** Size of cluster on lab axis. */
    float limitL;
    float limitA;
    float limitB;

    /**
      Maximum distance of two lab values.
     */
    float clusterSize;

    /**
      Stores tuples for fast access to nearest background/foreground pixels.
     */
    NearestPixelsMap nearestPixels;

public:
    /**
      Constructs a SioxSegmentator Object to be used for image segmentation.
     */
    KisSioxSegmentator(const KisPaintDeviceSP& pimage,
        float plimitL = L_DEFAULT_CLUSTER_SIZE,
        float plimitA = A_DEFAULT_CLUSTER_SIZE,
        float plimitB = B_DEFAULT_CLUSTER_SIZE);

    ~KisSioxSegmentator() {}

    bool segmentate(float cm[], int smoothness, double sizeFactorToKeep);

private:

    /**
      Function to fill up the confidence matrix based on a region growing
      algorithm. It sets <CODE>CERTAIN_FOREGROUND_CONFIDENCE</CODE> for
      corresponding areas of equal colors.

      \attention confidenceMatrix type is naive just for a first
      implementation abstraction. Is necessary to change to a compatible API
      type.
    */
    void fillColorRegions(float confidenceMatrix[]);

};
//#endif

#endif /* _KIS_SIOX_SEGMENTATOR_H_ */
