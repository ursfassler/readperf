
handname=handout
slidename=slides
bibname=literature
pdfprexif=perf_recording_
outdir=../generated/

quick: handout slide pdfclean
#	evince $(outdir)$(pdfprexif)$(handname).pdf &
#	evince $(outdir)$(pdfprexif)$(slidename).pdf &

handout:
#	-pdflatex -draftmode -halt-on-error $(handname).tex 1> /dev/null # mit dem - werden errors ignoriert
	-pdflatex -draftmode -halt-on-error $(handname).tex 1> /dev/null
	bibtex $(handname) 1> /dev/null
	pdflatex -draftmode -halt-on-error $(handname).tex 1> /dev/null
	pdflatex -halt-on-error $(handname).tex 1> /dev/null
	mv $(handname).pdf $(outdir)$(pdfprexif)$(handname).pdf

slide:
	-pdflatex -draftmode -halt-on-error $(slidename).tex 1> /dev/null
	bibtex $(slidename) 1> /dev/null
	pdflatex -draftmode -halt-on-error $(slidename).tex 1> /dev/null
	pdflatex -halt-on-error $(slidename).tex 1> /dev/null
	mv $(slidename).pdf $(outdir)$(pdfprexif)$(slidename).pdf

pdfclean:
	rm -f $(bibname)-blx.bib $(slidename)-blx.bib $(handname)-blx.bib *.log *.toc *.aux *.bbl *.blg *.lof *.lot *.out *.bak *.nav *.snm *.xml *.vrb

png: ps
	pstoimg -multi -type png -scale 2 -aaliastext -aaliastext $(handname).ps

clean: 
	rm -f *.dvi *.pdf *.ps *.png

