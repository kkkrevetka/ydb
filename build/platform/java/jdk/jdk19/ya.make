RESOURCES_LIBRARY()

DECLARE_EXTERNAL_HOST_RESOURCES_BUNDLE_BY_JSON(JDK19 jdk.json)
SET_RESOURCE_URI_FROM_JSON(WITH_JDK19_URI jdk.json)

IF (WITH_JDK19_URI)
    DECLARE_EXTERNAL_RESOURCE(WITH_JDK19 ${WITH_JDK19_URI})
ENDIF()

END()
