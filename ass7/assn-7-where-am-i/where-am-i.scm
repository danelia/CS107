;; The Where-Am-I helper functions:
;; originally written by Nick Parlante

;;
;;  the 2-D POINT type  (x and y coordinates)
;;  impelemented as a list length 2
;;
;;  the CIRCLE type (a radius and a center point)
;;  implemented as a list length 2-- first element is the radius and
;;  the second is the center point
;;
;;  For convenience, I do not insist that you treat these as ADT's-
;;  so if you want to use CAR to get the x coordinate, or build you own
;;  circles without going through MAKE-CIRCLE that will be ok.
;; 
;; POINT functions:
;;  make-pt   create a new point
;;  x         get the x coordinate of a point
;;  y         get the y coordinate of a point
;;  dist      return the distance between two points
;;
;; CIRCLE functions
;;  make-circle   create a new circle
;;  radius        get the radius of a circle
;;  center        get the center of a circle
;;  intersect     given two circles, returns a list of the points
;;                of intersection for those circles.
;;                For the purposes of this program I have
;;                bastardized the definition of 'intersect' a little
;;                to give better results when the measurements are
;;                inexact.  Don't worry about that, just use the points
;;                returned.  Someone who is interested in Math or Geometry
;;                might be interested to see how I compute the intersection.

(define (make-pt x y) 
  (list x y))

(define (x pt) 
  (car pt))

(define (y pt) 
  (cadr pt))

(define (dist pt1 pt2) 
  (let ((dx (- (x pt1) (x pt2)))
        (dy (- (y pt1) (y pt2))))
    (sqrt (+ (* dx dx) (* dy dy)))))

;;
;; 2D vector operations - used by the intersection function
;; vectors are a lot like points.  You won't need these.
;; 

(define (add v1 v2)
  (list (+ (car v1) (car v2))
	(+ (cadr v1) (cadr v2))))

(define (sub v1 v2)
  (list (- (car v1) (car v2))
	(- (cadr v1) (cadr v2))))

(define (len v)
  (sqrt (+ (* (car v) (car v))
	   (* (cadr v) (cadr v)))))

(define (scale v factor)
  (list (* (car v) factor) 
	(* (cadr v) factor)))

(define (normalize v)
  (scale (list (- (cadr v)) (car v)) (/ (len v))))


(define (make-circle radius center) 
  (list radius center))

(define (radius circle) 
  (car circle))

(define (center circle) 
  (cadr circle))

;;
;; Function: intersect
;; -------------------
;; Return a list of the points of intersection of the two circles.
;; The circles may not have the same center point
;;

(define (intersect circle1 circle2)
  (if (equal? (center circle1) (center circle2))
      (error "Intersect cannont handle circles with the same center point.")
      (let* ((c1 (center circle1))
	     (r1 (radius circle1))
	     (c2 (center circle2))
	     (r2 (radius circle2))
	     (d (dist c1 c2)))
      ;; first check to see if the circles are too far apart to intersect,
      ;; or if one circle is within another.
	(if (or (> d (+ r1 r2)) (> r1 (+ d r2)) (> r2 (+ d r1)))
	    ;; if there is no real intersection, use the closest tangent points on each
	    ;; circle.  This is the bastardization above.
	    (list (add c1 (scale (sub c2 c1) (/ r1 d)))  ;; c1-> towards c2
		  (add c2 (scale (sub c1 c2) (/ r2 d)))) ;; c2-> towards c1
    ;;otherwise the circles intersect normally, and I did some hairy
    ;;geometry to show that the following computes the two points
    ;;of intersection.
    (let* ((r12 (* r1 r1))
           (r22 (* r2 r2))
           (d2 (* d d))
           (d1 (/ (+ r12 (- r22) d2) 2 d))
           (h (sqrt (- r12 (* d1 d1))))
           (towards (scale (sub c2 c1) (/ d1 d))) ;;vector c1->c2
           (perp (scale (normalize towards) h)))
      (list (add c1 (add towards perp))
            (add c1 (add towards (scale perp -1)))))))))

;;
;; Function: prefix-of-list
;; ------------------------
;; Accepts the incoming list and returns one
;; with the same first k elements and nothing more.
;;

(define (prefix-of-list ls k)
  (if (or (zero? k) (null? ls)) '()
      (cons (car ls) (prefix-of-list (cdr ls) (- k 1)))))

;;
;; Function: partition
;; -------------------
;; Takes a pivot and a list and produces a pair two lists.
;; The first of the two lists contains all of those element less than the 
;; pivot, and the second contains everything else.  Notice that
;; the first list pair every produced is (() ()), and as the
;; recursion unwinds exactly one of the two lists gets a new element
;; cons'ed to the front of it.  
;; 

(define (partition pivot num-list)
  (if (null? num-list) '(() ())
      (let ((split-of-rest (partition pivot (cdr num-list))))
	(if (< (car num-list) pivot)
	    (list (cons (car num-list) (car split-of-rest)) (cadr split-of-rest))
	    (list (car split-of-rest) (cons (car num-list) (car (cdr split-of-rest))))))))

;;
;; Function: quicksort
;; -------------------
;; Implements the quicksort algorithm to sort lists of numbers from
;; high to low.  If a list is of length 0 or 1, then it is trivially
;; sorted.  Otherwise, we partition to cdr of the list around the car
;; to generate two lists: those in the cdr that are smaller than the car,
;; and those in the cdr that are greater than or equal to the car.  
;; We then recursively quicksort the two lists, and then splice everything
;; together in the proper order.
;;

(define (quicksort num-list)
  (if (<= (length num-list) 1) num-list
      (let ((split (partition (car num-list) (cdr num-list))))
	(append (quicksort (car split)) 
		(list (car num-list)) 
		(quicksort (cadr split))))))

;;
;; Function: remove
;; ----------------
;; Generates a copy of the specified list, except that all
;; instances that match the specified elem in the equal? sense
;; are excluded.
;;

(define (remove elem ls)
  (cond ((null? ls) '())
	((equal? (car ls) elem) (remove elem (cdr ls)))
	(else (cons (car ls) (remove elem (cdr ls))))))
                  
;; 
;; Function: all-guesses
;; ---------------------
;; Given a list of distances and a list of stars, return a list of all
;; the possible guesses.  A single guess is a list of circles which pairs
;; each distance with one of the stars.
;; 

(define (all-guesses distances stars)
  (if (or (null? distances) (null? stars)) '(())
      (apply append 
	     (map (lambda (star)
		    (map (lambda (pair) 
			   (cons (list (car distances) star) pair))
			 (all-guesses (cdr distances) (remove star stars))
			 )
		    )
		  stars))))

(define *distances-1* '(2.65 5.55 5.25))
(define *stars-1* '((0 0) (4 6) (10 0) (7 4) (12 5)))

(define *distances-2* '(2.5 11.65 7.75))
(define *stars-2* '((0 0) (4 4) (10 0)))

;; 
;; Include your code below.. make sure
;; write and test each function independently.  This
;; will require that you repeatedly type in
;; (load "where-am-i.scm") at the command prompt, allowing
;; it to override exisiting definitions and including
;; the most recently implemented into the lot.
;;







;;; --------------------------------------------------------------------------------------
;;; 				!!!!!!! TESTING PART !!!!!!!


;;; To test each:
;;;	(test-intersection-points) 
;;;	(test-distance-product)
;;;	(test-rate-points)
;;;	(test-sort-points)
;;;	(test-clumped-points)
;;;	(test-average-point)
;;;	(test-best-estimate)
;;;	(test-where-am-i))
;;; To test all:
;;;	(test-all)



;; !!! FOR TESTING ONLY
;; Function: test-subset?
;; ----------------
;; Checks if elements in
;; first sequence is in second sequence
;;
(define (test-subset? seq1 seq2)
	(if (null? seq1) #t
		(and (member (car seq1) seq2) 
			(test-subset?  (cdr seq1) seq2))))

;; !!! FOR TESTING ONLY
;; Function: test-subtest-ext-correct?
;; ----------------
;; Checks if 2 sequences are equal
;; Note first one must be set
;;
(define (test-subtest-ext-correct? seq1 seq2)
	(and (= (length seq1) (length seq2))
		(test-subset? seq1 seq2)))

;; !!! FOR TESTING ONLY
;; Function: test-b2i
;; ----------------
;; Converts boolean into number (true->1 false->0)
;;
(define (test-b2i b)
  (cadr (assq b '((#t 1) (#f 0)))) )

;; !!! FOR TESTING ONLY
;; Function: test-sum-bool
;; ----------------
;; evaluates sum of elem list
;;
(define (test-sum-bool elemList)
  	(if (null? elemList)  0
    		(+ (test-b2i (car elemList))
			(test-sum-bool (cdr elemList)))))

;; !!! FOR TESTING ONLY
;; Function: test-disp
;; ----------------
;; display test result with test name
;; seq must be sequence of booleans
;;
(define (test-disp test-name seq)
	(display (string-append ">>> On Test " test-name " :\t"
		(number->string (test-sum-bool seq)) "/" (number->string (length seq))
			" passed!!!\n")))

;; !!! FOR TESTING ONLY
;; Function: test 
;; ----------------
;; general test function
;; takes name of test, compare function, test function and 
;; input and output list
;;
(define (test test-name test-cmp test-fn test-in-out)
	(test-disp  test-name (map (lambda (elem) 
		(test-cmp (cadr elem) 
			(apply test-fn (car elem))))	test-in-out)))

;; TEST structure
;;(define (test-fun)
;;        (test "fun" equal?  fun 
;;	    '(  (   ( input        ) 	output )
;;		(   ( input        ) 	output ) )))

;; TEST intersection-points
(define (test-intersection-points)
          (test "intersection-points" test-subtest-ext-correct?  intersection-points
              '(((((1 (0 0)) (1 (1 0))))	
                     ((0.5 0.8660254037844386) (0.5 -0.8660254037844386)) )
               ((((1 (0 0)) (1 (1 0)) (1 (1 1))))
                     ((0.5 0.8660254037844386) (0.5 -0.8660254037844386)
                     (2.7755575615628914E-16 1.0) (1.0 2.7755575615628914E-16)
                     (0.1339745962155614 0.5) (1.8660254037844386 0.5))))))

;; TEST distance-product
(define (test-distance-product)
        (test "distance-product" equal?  distance-product 
           '((((2 0) ((0 0) (2 0) (6 0)))           8.0 )
            (((3 3) ((2 5) (7 8) (10 1) (3 2)))  104.23531071570709 ) )))

;; TEST rate-points
(define (test-rate-points)
	(test "rate-points" equal?  rate-points 
            '(((((0 0) (2 0) (6 0))) 	
                      ((12.0 (0 0)) (8.0 (2 0)) (24.0 (6 0))))
             ((((2 5) (7 8) (10 1) (3 2)))        
                      ((164.92422502470643 (2 5)) (320.22492095400696 (7 8))
                      (481.66378315169186 (10 1)) (161.24515496597098 (3 2)))))))
	
;; TEST sort-points
(define (test-sort-points)
        (test "sort-points" equal?  sort-points  
            '(((((164.92422502470643 (2 5)) (320.22492095400696 (7 8))
                        (481.66378315169186 (10 1)) (161.24515496597098 (3 2)))) 
                        ((161.24515496597098 (3 2)) (164.92422502470643 (2 5))
                        (320.22492095400696 (7 8)) (481.66378315169186 (10 1))))
             ((((12.0 (0 0)) (8.0 (2 0)) (24.0 (6 0))))
                        ((8.0 (2 0)) (12.0 (0 0)) (24.0 (6 0)))))))

;; TEST clumped-points
(define (test-clumped-points)
        (test "clumped-points" equal?  clumped-points 
               '(((((0 0) (2 0) (6 0)))         ((2 0)))
                ((((0 0) (2 0) (6 0) (1 0))) 	((1 0) (2 0))))))

;; TEST average-point
(define (test-average-point)
        (test "average-point" equal?  average-point 
           '(((((0 0) (2 0) (6 0))) 	
                        (5.925925925925926 (8/3 0)))
            ((((0 0) (2 0) (6 0) (1 0) (5 4) (4 5))) 	
                        (590.8865213418864 (3 3/2))))))

;; TEST best-estimate
(define (test-best-estimate)
          (test "best-estimate" equal?  best-estimate 
           '(((((1 (0 0)) (1 (2 0)) (0.1 (1 0))) )  
                (5.942527670663184E-4 (1.0016666666666667 0.033291640592396886)))
            ((((1 (0 0)) (1 (2 0)) (0.1 (1 4)))) 	
                (2.0597636878043715 (0.747511875012111 2.9253713333720888))))))

;; TEST where-am-i
(define (test-where-am-i)
        (test "where-am-i" equal?  where-am-i
           '((((2.5 11.65 7.75) ((0 0) (4 4) (10 0)) ) 
               ((5.164102748844367E-6 (11.481441859657613 2.001220110464802))
               (0.3394092159986836 (-1.8429290506186957 -1.216560811506545))
               (0.6676116235553851 (7.76704142138513 0.4622501635210244))
               (0.7871787994250546 (2.128838984322123 0.5892556509887895))
               (4.398427430402362 (3.9811875000000003 6.126803974552016))
               (45.38616651704703 (4.326820849071189 4.1540809322972985))))
            (((1 2) ((0 0) (3 0)))
             ((1 (1.0 0.0)) (1 (2.0 0.0)))))))

;; TEST ALL
(define (test-all)
	(test-intersection-points) 
	(test-distance-product)
	(test-rate-points)
	(test-sort-points)
	(test-clumped-points)
	(test-average-point)
	(test-best-estimate)
	(test-where-am-i))
