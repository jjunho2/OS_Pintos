set number
set autoindent
set smartindent
set cindent
set shiftwidth=4
set tabstop=4
set ignorecase
set hlsearch
set expandtab
set nocompatible
set bs=indent,eol,start
set history=512
set ruler
set nobackup
set title
set showmatch
set nowrap
set wmnu
set scrolloff=2
set wildmode=longest,list
set ts=4
set sts=4
set autowrite
set autoread
set laststatus=2
set paste
set softtabstop=4
set incsearch
set statusline=\ %<%l:%v\ [%P]%=%a\ %h%m%r\ %F\

au BufReadPost *
\ if line ("'\"") > 0 && line("'\"") <= line("$") |
\ exe "norm g`\"" |
\ endif

syntax on
