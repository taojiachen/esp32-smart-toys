idf_component_register(SRC_DIRS 
                                "."
                                "app"
                                "task"
                    INCLUDE_DIRS 
                                "."
                                "app"
                                "task")
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)