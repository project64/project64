DOCNAME=proposal
SOURCES = $(wildcard *.tex) $(wildcard text/*.tex) $(wildcard figs/*.pdf) \
	$(wildcard figs/*.jpg) $(wildcard *.bib)

all: $(DOCNAME).pdf 

$(DOCNAME).pdf: $(SOURCES) $(DOCNAME).bbl
	-pdflatex $(LATEXFLAGS) $(DOCNAME) 
	-bibtex $(DOCNAME) 
	-pdflatex $(LATEXFLAGS) $(DOCNAME) 
	-pdflatex $(LATEXFLAGS) $(DOCNAME)

$(DOCNAME).bbl: $(wildcard *.bib)
	-pdflatex $(LATEXFLAGS) $(DOCNAME)
	-bibtex $(DOCNAME) 
	-pdflatex $(LATEXFLAGS) $(DOCNAME)

clean:
	$(RM) $(DOCNAME).bbl $(DOCNAME).brf $(DOCNAME).aux $(DOCNAME).dvi $(DOCNAME).ps $(DOCNAME).pdf *.log $(DOCNAME).blg 

