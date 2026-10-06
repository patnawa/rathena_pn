-- Required for atomic auction settlement. Back up first and stop game writers.
-- Keep feature.auction off: map-to-char registration/bid recovery is separate.
ALTER TABLE `auction` ENGINE=InnoDB;
