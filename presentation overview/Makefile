
handname=handout
slidename=slides
bibname=literature
pdfprexif=perf_

quick: handout slide pdfclean
	evince $(pdfprexif)$(handname).pdf &
	evince $(pdfprexif)$(slidename).pdf &

handout:
#	-pdflatex -draftmode -halt-on-error $(handname).tex 1> /dev/null # mit dem - werden errors ignoriert
	-pdflatex -draftmode -halt-on-error $(handname).tex 1> /dev/null
	bibtex $(handname) 1> /dev/null
	pdflatex -draftmode -halt-on-error $(handname).tex 1> /dev/null
	pdflatex -halt-on-error $(handname).tex 1> /dev/null
	mv $(handname).pdf $(pdfprexif)$(handname).pdf

slide:
	-pdflatex -draftmode -halt-on-error $(slidename).tex 1> /dev/null
	bibtex $(slidename) 1> /dev/null
	pdflatex -draftmode -halt-on-error $(slidename).tex 1> /dev/null
	pdflatex -halt-on-error $(slidename).tex 1> /dev/null
	mv $(slidename).pdf $(pdfprexif)$(slidename).pdf

pdfclean:
	rm -f $(bibname)-blx.bib *.log *.toc *.aux *.bbl *.blg *.lof *.lot *.out *.bak *.nav *.snm

png: ps
	pstoimg -multi -type png -scale 2 -aaliastext -aaliastext $(handname).ps

clean: 
	rm -f *.dvi *.pdf *.ps *.png

