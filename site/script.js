// Smooth scrolling for navigation links
document.addEventListener('DOMContentLoaded', function() {
    // Smooth scrolling for navigation links
    const navLinks = document.querySelectorAll('.nav a[href^="#"]');
    
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            
            const targetId = this.getAttribute('href');
            const targetSection = document.querySelector(targetId);
            
            if (targetSection) {
                const headerHeight = document.querySelector('.header').offsetHeight;
                const targetPosition = targetSection.offsetTop - headerHeight - 20;
                
                window.scrollTo({
                    top: targetPosition,
                    behavior: 'smooth'
                });
            }
        });
    });

    // Add active class to navigation links based on scroll position
    const sections = document.querySelectorAll('section[id]');
    const navItems = document.querySelectorAll('.nav a[href^="#"]');

    function updateActiveNav() {
        const scrollPosition = window.scrollY + 100;

        sections.forEach(section => {
            const sectionTop = section.offsetTop;
            const sectionHeight = section.offsetHeight;
            const sectionId = section.getAttribute('id');

            if (scrollPosition >= sectionTop && scrollPosition < sectionTop + sectionHeight) {
                navItems.forEach(item => {
                    item.classList.remove('active');
                    if (item.getAttribute('href') === `#${sectionId}`) {
                        item.classList.add('active');
                    }
                });
            }
        });
    }

    window.addEventListener('scroll', updateActiveNav);

    // Add hover effects to command cards
    const commandCards = document.querySelectorAll('.command');
    
    commandCards.forEach(card => {
        card.addEventListener('mouseenter', function() {
            this.style.backgroundColor = '#2a0000';
            this.style.transform = 'translateX(5px)';
        });
        
        card.addEventListener('mouseleave', function() {
            this.style.backgroundColor = '#0a0a0a';
            this.style.transform = 'translateX(0)';
        });
    });

    // Add hover effects to feature cards
    const featureCards = document.querySelectorAll('.feature-card');
    
    featureCards.forEach(card => {
        card.addEventListener('mouseenter', function() {
            this.style.transform = 'translateY(-5px)';
        });
        
        card.addEventListener('mouseleave', function() {
            this.style.transform = 'translateY(0)';
        });
    });

    // Add copy functionality to code blocks
    const codeBlocks = document.querySelectorAll('.code-block');
    
    codeBlocks.forEach(block => {
        const copyButton = document.createElement('button');
        copyButton.innerHTML = 'Copy';
        copyButton.className = 'copy-button';
        copyButton.style.cssText = `
            position: absolute;
            top: 10px;
            right: 10px;
            background: #cc0000;
            color: white;
            border: none;
            padding: 5px 10px;
            border-radius: 3px;
            cursor: pointer;
            font-size: 12px;
            opacity: 0;
            transition: opacity 0.3s ease;
        `;
        
        block.style.position = 'relative';
        block.appendChild(copyButton);
        
        block.addEventListener('mouseenter', () => {
            copyButton.style.opacity = '1';
        });
        
        block.addEventListener('mouseleave', () => {
            copyButton.style.opacity = '0';
        });
        
        copyButton.addEventListener('click', () => {
            const code = block.querySelector('code').textContent;
            navigator.clipboard.writeText(code).then(() => {
                copyButton.innerHTML = 'Copied!';
                setTimeout(() => {
                    copyButton.innerHTML = 'Copy';
                }, 2000);
            });
        });
    });

    // Add search functionality for commands
    const searchInput = document.createElement('input');
    searchInput.placeholder = 'Search commands...';
    searchInput.style.cssText = `
        width: 100%;
        max-width: 400px;
        padding: 10px;
        margin: 20px auto;
        display: block;
        background: #1a1a1a;
        border: 1px solid #333;
        border-radius: 4px;
        color: white;
        font-family: 'Courier New', monospace;
    `;

    const commandsSection = document.querySelector('#commands .container');
    const commandsTitle = commandsSection.querySelector('h2');
    commandsTitle.after(searchInput);

    searchInput.addEventListener('input', function() {
        const searchTerm = this.value.toLowerCase();
        const commands = document.querySelectorAll('.command');
        
        commands.forEach(command => {
            const commandText = command.textContent.toLowerCase();
            const commandCategory = command.closest('.command-category');
            
            if (commandText.includes(searchTerm)) {
                command.style.display = 'flex';
            } else {
                command.style.display = 'none';
            }
        });
        
        // Hide categories with no visible commands
        const categories = document.querySelectorAll('.command-category');
        categories.forEach(category => {
            const visibleCommands = category.querySelectorAll('.command[style*="flex"]');
            if (visibleCommands.length === 0 && searchTerm !== '') {
                category.style.display = 'none';
            } else {
                category.style.display = 'block';
            }
        });
    });

    // Add theme toggle (optional enhancement)
    const themeToggle = document.createElement('button');
    themeToggle.innerHTML = 'Toggle Theme';
    themeToggle.style.cssText = `
        position: fixed;
        bottom: 20px;
        right: 20px;
        background: #cc0000;
        color: white;
        border: none;
        padding: 10px 15px;
        border-radius: 50px;
        cursor: pointer;
        z-index: 1000;
        font-family: 'Courier New', monospace;
        display: none;
    `;
    
    document.body.appendChild(themeToggle);

    // Add loading animation for page
    window.addEventListener('load', function() {
        document.body.style.opacity = '0';
        document.body.style.transition = 'opacity 0.5s ease';
        
        setTimeout(() => {
            document.body.style.opacity = '1';
        }, 100);
    });

    // Add keyboard shortcuts
    document.addEventListener('keydown', function(e) {
        // Ctrl + / to focus search
        if (e.ctrlKey && e.key === '/') {
            e.preventDefault();
            searchInput.focus();
        }
        
        // Escape to clear search
        if (e.key === 'Escape' && document.activeElement === searchInput) {
            searchInput.value = '';
            searchInput.dispatchEvent(new Event('input'));
            searchInput.blur();
        }
    });

    console.log('mxdbg website loaded successfully!');
});