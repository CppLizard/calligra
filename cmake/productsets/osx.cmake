#defines the set of products commonly wanted for classic Desktop environment with OS X
# TODO: platform specific things should be handled in toplevel CMakeLists.txt

calligra_define_productset(OSX "Calligra for OS X Desktop"
    OPTIONAL
        # apps
        BRAINDUMP
        FLOW
        KARBON
        PLAN
        SHEETS
        STAGE
        WORDS
        # features
        FEATURE_SCRIPTING
        FEATURE_RDF
        # extras
        APP_CALLIGRA
        APP_CONVERTER
        FILEMANAGER
        OKULAR
        # docs
        DOC
)
