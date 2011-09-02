
texname=main
outname=report
tmpfiles=$(texname)-blx.bib *.log *.toc *.bbl *.blg *.lof *.lot *.out *.bak *.idx *.ilg *.ind *.lol *.xml

pdf:
	pdflatex -draftmode -halt-on-error $(texname).tex 1> /dev/null
	bibtex $(texname)
	pdflatex -draftmode -halt-on-error $(texname).tex 1> /dev/null
	pdflatex -halt-on-error $(texname).tex 1> /dev/null
	mv $(texname).pdf ../generated/$(outname).pdf
	rm -f $(tmpfiles)
	find ./ -iname "*.aux" -exec "rm" '{}' ';'

clean: 
	rm -f $(texname).dvi $(texname).pdf $(texname).ps $(outname).dvi $(outname).pdf $(outname).ps

